#include "schema_original.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>

// Inserts reference rows (node, process, threads, agents, queues, streams, code
// objects, kernels) required before kernel dispatch rows can be loaded.
void populateOriginalRefs(sqlite3* db, const RefData& refs) {
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

    // Threads
    for (const auto& [id, tid] : refs.threads) {
        snprintf(sql, sizeof(sql),
                 "INSERT INTO rocpd_info_thread (id, nid, pid, tid) VALUES (%d, 1, 1, %d);",
                 id, tid);
        sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
    }

    // Agents
    for (const auto& [id, info] : refs.agents) {
        snprintf(sql, sizeof(sql),
                 "INSERT INTO rocpd_info_agent (id, nid, pid, type, name) VALUES (%d, 1, 1, '%s', '%s');",
                 id, info.first.c_str(), info.second.c_str());
        sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
    }

    // Queues
    for (const auto& [id, name] : refs.queues) {
        snprintf(sql, sizeof(sql),
                 "INSERT INTO rocpd_info_queue (id, nid, pid, name) VALUES (%d, 1, 1, '%s');",
                 id, name.c_str());
        sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
    }

    // Streams
    for (const auto& [id, name] : refs.streams) {
        snprintf(sql, sizeof(sql),
                 "INSERT INTO rocpd_info_stream (id, nid, pid, name) VALUES (%d, 1, 1, '%s');",
                 id, name.c_str());
        sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
    }

    // Code objects
    for (const auto& [id, uri] : refs.code_objects) {
        snprintf(sql, sizeof(sql),
                 "INSERT INTO rocpd_info_code_object (id, nid, pid, uri) VALUES (%d, 1, 1, '%s');",
                 id, uri.c_str());
        sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
    }

    // Kernels
    for (const auto& [id, info] : refs.kernels) {
        // Escape single quotes in kernel name
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

    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
}

namespace {

constexpr int kOriginalDispatchBindsPerRow = 14;
// Cap batch size below SQLITE_LIMIT_VARIABLE_NUMBER (999); 64 * 14 = 896.
constexpr size_t kOriginalDispatchChunkRows = 64;

// Inserts each dispatch with a single-row prepared statement (bind, step, reset).
static void insert_original_dispatches_one_by_one(
    sqlite3* db, const std::vector<KernelDispatch>& dispatches) {
    sqlite3_stmt* stmt;
    const char* sql =
        "INSERT INTO rocpd_kernel_dispatch "
        "(nid, pid, tid, agent_id, kernel_id, dispatch_id, queue_id, stream_id, "
        "start, \"end\", workgroup_size_x, workgroup_size_y, workgroup_size_z, "
        "grid_size_x, grid_size_y, grid_size_z) "
        "VALUES (1, 1, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    for (const auto& d : dispatches) {
        sqlite3_bind_int(stmt, 1, d.tid);
        sqlite3_bind_int(stmt, 2, d.agent_id);
        sqlite3_bind_int(stmt, 3, d.kernel_id);
        sqlite3_bind_int(stmt, 4, d.dispatch_id);
        sqlite3_bind_int(stmt, 5, d.queue_id);
        sqlite3_bind_int(stmt, 6, d.stream_id);
        sqlite3_bind_int64(stmt, 7, d.start);
        sqlite3_bind_int64(stmt, 8, d.end);
        sqlite3_bind_int(stmt, 9, d.workgroup_size_x);
        sqlite3_bind_int(stmt, 10, d.workgroup_size_y);
        sqlite3_bind_int(stmt, 11, d.workgroup_size_z);
        sqlite3_bind_int(stmt, 12, d.grid_size_x);
        sqlite3_bind_int(stmt, 13, d.grid_size_y);
        sqlite3_bind_int(stmt, 14, d.grid_size_z);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }

    sqlite3_finalize(stmt);
}

// Rows per multi-row INSERT: min(64, SQLITE_LIMIT_VARIABLE_NUMBER / binds per row).
static size_t chunk_row_count_original(sqlite3* db) {
    int lim = sqlite3_limit(db, SQLITE_LIMIT_VARIABLE_NUMBER, -1);
    if (lim < kOriginalDispatchBindsPerRow) {
        return 1;
    }
    const size_t max_by_limit =
        static_cast<size_t>(lim / kOriginalDispatchBindsPerRow);
    return std::min(kOriginalDispatchChunkRows, max_by_limit);
}

// Builds INSERT ... VALUES (row), (row), ... with the given number of placeholder rows.
static std::string build_original_multi_insert_sql(size_t row_count) {
    static const char kPrefix[] =
        "INSERT INTO rocpd_kernel_dispatch "
        "(nid, pid, tid, agent_id, kernel_id, dispatch_id, queue_id, stream_id, "
        "start, \"end\", workgroup_size_x, workgroup_size_y, workgroup_size_z, "
        "grid_size_x, grid_size_y, grid_size_z) VALUES ";
    std::string sql;
    sql.reserve(sizeof(kPrefix) + row_count * 60);
    sql = kPrefix;
    for (size_t r = 0; r < row_count; ++r) {
        if (r > 0) {
            sql += ", ";
        }
        sql += "(1, 1, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    }
    return sql;
}

// Binds `count` consecutive dispatches starting at `offset` into a multi-row stmt.
static void bind_original_chunk(sqlite3_stmt* stmt,
                                const std::vector<KernelDispatch>& dispatches, size_t offset,
                                size_t count) {
    int idx = 1;
    for (size_t r = 0; r < count; ++r) {
        const auto& d = dispatches[offset + r];
        sqlite3_bind_int(stmt, idx++, d.tid);
        sqlite3_bind_int(stmt, idx++, d.agent_id);
        sqlite3_bind_int(stmt, idx++, d.kernel_id);
        sqlite3_bind_int(stmt, idx++, d.dispatch_id);
        sqlite3_bind_int(stmt, idx++, d.queue_id);
        sqlite3_bind_int(stmt, idx++, d.stream_id);
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
static void insert_original_dispatches_chunked(
    sqlite3* db, const std::vector<KernelDispatch>& dispatches) {
    const size_t n = chunk_row_count_original(db);
    if (n == 1) {
        insert_original_dispatches_one_by_one(db, dispatches);
        return;
    }

    const std::string sql_full = build_original_multi_insert_sql(n);
    sqlite3_stmt* stmt_full = nullptr;
    sqlite3_prepare_v2(db, sql_full.c_str(), static_cast<int>(sql_full.size()),
                       &stmt_full, nullptr);

    size_t offset = 0;
    for (; offset + n <= dispatches.size(); offset += n) {
        bind_original_chunk(stmt_full, dispatches, offset, n);
        sqlite3_step(stmt_full);
        sqlite3_reset(stmt_full);
        sqlite3_clear_bindings(stmt_full);
    }
    sqlite3_finalize(stmt_full);

    if (offset < dispatches.size()) {
        const size_t m = dispatches.size() - offset;
        const std::string sql_tail = build_original_multi_insert_sql(m);
        sqlite3_stmt* stmt_tail = nullptr;
        sqlite3_prepare_v2(db, sql_tail.c_str(), static_cast<int>(sql_tail.size()),
                             &stmt_tail, nullptr);
        bind_original_chunk(stmt_tail, dispatches, offset, m);
        sqlite3_step(stmt_tail);
        sqlite3_finalize(stmt_tail);
    }
}

}  // namespace

// Wraps dispatch insert in a transaction; uses chunked or one-by-one path.
void insertOriginalDispatches(sqlite3* db, const std::vector<KernelDispatch>& dispatches,
                              bool chunk_inserts) {
    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
    if (chunk_inserts) {
        insert_original_dispatches_chunked(db, dispatches);
    } else {
        insert_original_dispatches_one_by_one(db, dispatches);
    }
    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
}

// Benchmark read path: JOIN + ORDER BY; return value aggregates a column so the work stays live.
int64_t readOriginalDispatches(sqlite3* db) {
    sqlite3_stmt* stmt;
    const char* sql =
        "SELECT d.id, d.nid, d.pid, d.tid, d.agent_id, d.kernel_id, "
        "d.dispatch_id, d.queue_id, d.stream_id, d.start, d.\"end\", "
        "k.kernel_name "
        "FROM rocpd_kernel_dispatch d "
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

int64_t readRocpdSourceDispatchJoin(sqlite3* db) {
    char sql[768];
    snprintf(sql, sizeof(sql),
             "SELECT d.id, d.nid, d.pid, d.tid, d.agent_id, d.kernel_id, "
             "d.dispatch_id, d.queue_id, d.stream_id, d.start, d.\"end\", k.kernel_name "
             "FROM rocpd_kernel_dispatch_%s d "
             "JOIN rocpd_info_kernel_symbol_%s k ON d.kernel_id = k.id "
             "ORDER BY d.start",
             GUID, GUID);
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, static_cast<int>(strlen(sql)), &stmt, nullptr) !=
        SQLITE_OK) {
        return 0;
    }
    int64_t sum = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        sum += sqlite3_column_int64(stmt, 9);
    }
    sqlite3_finalize(stmt);
    return sum;
}

int64_t countRocpdSourceDispatches(sqlite3* db) {
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT COUNT(*) FROM rocpd_kernel_dispatch_%s", GUID);
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, static_cast<int>(strlen(sql)), &stmt, nullptr) !=
        SQLITE_OK) {
        return 0;
    }
    int64_t n = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        n = sqlite3_column_int64(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return n;
}

void createRocpdSourceSchemaPerformanceIndexes(sqlite3* db) {
    char tbl[160];
    snprintf(tbl, sizeof(tbl), "rocpd_kernel_dispatch_%s", GUID);
    struct {
        const char* cols;
        const char* name;
    } idx[] = {
        {"(tid)", "tid"},
        {"(agent_id)", "agent"},
        {"(kernel_id)", "kernel"},
        {"(queue_id)", "queue"},
        {"(stream_id)", "stream"},
        {"(start, kernel_id)", "start_kernel"},
        {"(tid, start)", "tid_start"},
    };
    char sql[384];
    for (const auto& i : idx) {
        snprintf(sql, sizeof(sql),
                 "CREATE INDEX IF NOT EXISTS idx_src_dispatch_%s ON %s%s", i.name, tbl,
                 i.cols);
        sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
    }
}
