#include "schema_original.h"
#include <cstdio>
#include <cstring>

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

void insertOriginalDispatches(sqlite3* db, const std::vector<KernelDispatch>& dispatches) {
    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

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
    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
}

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
