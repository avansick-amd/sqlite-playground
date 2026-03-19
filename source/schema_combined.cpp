#include "schema_combined.h"
#include <cstdio>
#include <cstring>
#include <unordered_set>

// Context mapping for combined schema
static std::unordered_map<std::string, int> g_context_map;

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

void insertCombinedDispatches(sqlite3* db, const std::vector<KernelDispatch>& dispatches) {
    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

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
    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
}

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
