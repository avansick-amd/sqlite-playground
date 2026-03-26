#include "schema_combined.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_set>

// Maps "tid_agent_queue_stream" keys to rocpd_dispatch_context ids for insert paths.
static std::unordered_map<std::string, int> g_context_map;

// Inserts reference rows and builds dispatch contexts from unique (tid, agent, queue, stream)
// tuples in `dispatches`; fills g_context_map for subsequent dispatch inserts.
void populateCombinedRefs(sqlite3* db, const RefData& refs, const std::vector<KernelDispatch>& dispatches) {
    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

    char sql[4096];

    // Node
    snprintf(sql, sizeof(sql),
             "INSERT INTO rocpd_info_node (hash, machine_id) VALUES (%lld, '%s');",
             (long long)refs.node_hash, refs.machine_id.c_str());
    sqlite3_exec(db, sql, nullptr, nullptr, nullptr);

    // Process
    snprintf(sql, sizeof(sql),
             "INSERT INTO rocpd_info_process (nid, pid) VALUES (1, %d);",
             refs.process_pid);
    sqlite3_exec(db, sql, nullptr, nullptr, nullptr);

    // Code objects
    for (const auto& [id, uri] : refs.code_objects) {
        snprintf(sql, sizeof(sql),
                 "INSERT INTO rocpd_info_code_object (id, nid, pid, uri) VALUES (%d, 1, 1, '%s');",
                 id, uri.c_str());
        sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
    }

    // Kernels
    for (const auto& [id, info] : refs.kernels) {
        std::string escaped_name;
        for (char c : info.second) {
            if (c == '\'')
                escaped_name += "''";
            else
                escaped_name += c;
        }
        snprintf(sql, sizeof(sql),
                 "INSERT INTO rocpd_info_kernel_symbol (id, nid, pid, code_object_id, kernel_name) "
                 "VALUES (%d, 1, 1, %d, '%s');",
                 id, info.first, escaped_name.c_str());
        sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
    }

    // Create unique contexts from dispatches
    g_context_map.clear();
    std::unordered_set<std::string> unique_contexts;
    for (const auto& d : dispatches) {
        char key[256];
        snprintf(key, sizeof(key), "%d_%d_%d_%d", d.tid, d.agent_id, d.queue_id, d.stream_id);
        unique_contexts.insert(key);
    }

    int ctx_id = 1;
    for (const auto& key : unique_contexts) {
        int tid, agent_id, queue_id, stream_id;
        sscanf(key.c_str(), "%d_%d_%d_%d", &tid, &agent_id, &queue_id, &stream_id);

        auto& agent = refs.agents.at(agent_id);
        auto& queue_name = refs.queues.at(queue_id);
        auto& stream_name = refs.streams.at(stream_id);
        int tid_val = refs.threads.at(tid);

        snprintf(sql, sizeof(sql),
                 "INSERT INTO rocpd_dispatch_context "
                 "(id, nid, pid, tid, agent_id, agent_type, agent_name, queue_id, "
                 "queue_name, stream_id, stream_name) "
                 "VALUES (%d, 1, 1, %d, %d, '%s', '%s', %d, '%s', %d, '%s');",
                 ctx_id, tid_val, agent_id, agent.first.c_str(),
                 agent.second.c_str(), queue_id, queue_name.c_str(), stream_id,
                 stream_name.c_str());
        sqlite3_exec(db, sql, nullptr, nullptr, nullptr);

        g_context_map[key] = ctx_id;
        ctx_id++;
    }

    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
}

namespace {

constexpr int kCombinedDispatchBindsPerRow = 11;
// Cap below SQLITE_LIMIT_VARIABLE_NUMBER; 64 * 11 = 704.
constexpr size_t kCombinedDispatchChunkRows = 64;

// Resolves context_id via g_context_map then inserts one row per dispatch (bind, step, reset).
static void insert_combined_dispatches_one_by_one(
    sqlite3* db, const std::vector<KernelDispatch>& dispatches) {
    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO rocpd_kernel_dispatch "
                      "(context_id, kernel_id, dispatch_id, start, \"end\", "
                      "workgroup_size_x, workgroup_size_y, workgroup_size_z, "
                      "grid_size_x, grid_size_y, grid_size_z) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    char key[256];
    for (const auto& d : dispatches) {
        snprintf(key, sizeof(key), "%d_%d_%d_%d", d.tid, d.agent_id, d.queue_id, d.stream_id);
        int ctx_id = g_context_map[key];

        sqlite3_bind_int(stmt, 1, ctx_id);
        sqlite3_bind_int(stmt, 2, d.kernel_id);
        sqlite3_bind_int(stmt, 3, d.dispatch_id);
        sqlite3_bind_int64(stmt, 4, d.start);
        sqlite3_bind_int64(stmt, 5, d.end);
        sqlite3_bind_int(stmt, 6, d.workgroup_size_x);
        sqlite3_bind_int(stmt, 7, d.workgroup_size_y);
        sqlite3_bind_int(stmt, 8, d.workgroup_size_z);
        sqlite3_bind_int(stmt, 9, d.grid_size_x);
        sqlite3_bind_int(stmt, 10, d.grid_size_y);
        sqlite3_bind_int(stmt, 11, d.grid_size_z);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }

    sqlite3_finalize(stmt);
}

// Rows per multi-row INSERT: min(64, SQLITE_LIMIT_VARIABLE_NUMBER / binds per row).
static size_t chunk_row_count_combined(sqlite3* db) {
    int lim = sqlite3_limit(db, SQLITE_LIMIT_VARIABLE_NUMBER, -1);
    if (lim < kCombinedDispatchBindsPerRow) {
        return 1;
    }
    const size_t max_by_limit =
        static_cast<size_t>(lim / kCombinedDispatchBindsPerRow);
    return std::min(kCombinedDispatchChunkRows, max_by_limit);
}

// Builds INSERT ... VALUES (row), (row), ... with the given number of placeholder rows.
static std::string build_combined_multi_insert_sql(size_t row_count) {
    static const char kPrefix[] =
        "INSERT INTO rocpd_kernel_dispatch "
        "(context_id, kernel_id, dispatch_id, start, \"end\", "
        "workgroup_size_x, workgroup_size_y, workgroup_size_z, "
        "grid_size_x, grid_size_y, grid_size_z) VALUES ";
    std::string sql;
    sql.reserve(sizeof(kPrefix) + row_count * 50);
    sql = kPrefix;
    for (size_t r = 0; r < row_count; ++r) {
        if (r > 0) {
            sql += ", ";
        }
        sql += "(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    }
    return sql;
}

// Binds `count` consecutive dispatches starting at `offset` (context_id from g_context_map).
static void bind_combined_chunk(sqlite3_stmt* stmt,
                                const std::vector<KernelDispatch>& dispatches, size_t offset,
                                size_t count) {
    char key[256];
    int idx = 1;
    for (size_t r = 0; r < count; ++r) {
        const auto& d = dispatches[offset + r];
        snprintf(key, sizeof(key), "%d_%d_%d_%d", d.tid, d.agent_id, d.queue_id,
                 d.stream_id);
        int ctx_id = g_context_map[key];

        sqlite3_bind_int(stmt, idx++, ctx_id);
        sqlite3_bind_int(stmt, idx++, d.kernel_id);
        sqlite3_bind_int(stmt, idx++, d.dispatch_id);
        sqlite3_bind_int64(stmt, idx++, d.start);
        sqlite3_bind_int64(stmt, idx++, d.end);
        sqlite3_bind_int(stmt, idx++, d.workgroup_size_x);
        sqlite3_bind_int(stmt, idx++, d.workgroup_size_y);
        sqlite3_bind_int(stmt, idx++, d.workgroup_size_z);
        sqlite3_bind_int(stmt, idx++, d.grid_size_x);
        sqlite3_bind_int(stmt, idx++, d.grid_size_y);
        sqlite3_bind_int(stmt, idx++, d.grid_size_z);
    }
}

// Multi-row INSERT in chunks of n: reuse one prepared stmt for full chunks; one-off
// stmt for the final partial chunk. Falls back to one-by-one if n == 1.
static void insert_combined_dispatches_chunked(
    sqlite3* db, const std::vector<KernelDispatch>& dispatches) {
    const size_t n = chunk_row_count_combined(db);
    if (n == 1) {
        insert_combined_dispatches_one_by_one(db, dispatches);
        return;
    }

    const std::string sql_full = build_combined_multi_insert_sql(n);
    sqlite3_stmt* stmt_full = nullptr;
    sqlite3_prepare_v2(db, sql_full.c_str(), static_cast<int>(sql_full.size()),
                       &stmt_full, nullptr);

    size_t offset = 0;
    for (; offset + n <= dispatches.size(); offset += n) {
        bind_combined_chunk(stmt_full, dispatches, offset, n);
        sqlite3_step(stmt_full);
        sqlite3_reset(stmt_full);
        sqlite3_clear_bindings(stmt_full);
    }
    sqlite3_finalize(stmt_full);

    if (offset < dispatches.size()) {
        const size_t m = dispatches.size() - offset;
        const std::string sql_tail = build_combined_multi_insert_sql(m);
        sqlite3_stmt* stmt_tail = nullptr;
        sqlite3_prepare_v2(db, sql_tail.c_str(), static_cast<int>(sql_tail.size()),
                           &stmt_tail, nullptr);
        bind_combined_chunk(stmt_tail, dispatches, offset, m);
        sqlite3_step(stmt_tail);
        sqlite3_finalize(stmt_tail);
    }
}

}  // namespace

// Wraps dispatch insert in a transaction; uses chunked or one-by-one path.
void insertCombinedDispatches(sqlite3* db, const std::vector<KernelDispatch>& dispatches,
                              bool chunk_inserts) {
    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
    if (chunk_inserts) {
        insert_combined_dispatches_chunked(db, dispatches);
    } else {
        insert_combined_dispatches_one_by_one(db, dispatches);
    }
    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
}

// Benchmark read path: JOINs + ORDER BY; return value aggregates a column so the work stays live.
int64_t readCombinedDispatches(sqlite3* db) {
    sqlite3_stmt* stmt;
    const char* sql =
        "SELECT d.id, c.nid, c.pid, c.tid, c.agent_id, d.kernel_id, "
        "d.dispatch_id, c.queue_id, c.stream_id, d.start, d.\"end\", "
        "k.kernel_name "
        "FROM rocpd_kernel_dispatch d "
        "JOIN rocpd_dispatch_context c ON d.context_id = c.id "
        "JOIN rocpd_info_kernel_symbol k ON d.kernel_id = k.id "
        "ORDER BY d.start";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    int64_t sum = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        sum += sqlite3_column_int64(stmt, 9);
    }
    sqlite3_finalize(stmt);
    return sum;
}
