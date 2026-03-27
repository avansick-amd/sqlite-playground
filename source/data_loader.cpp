#include "data_loader.h"
#include <sqlite3.h>
#include <cstdio>
#include <cstring>

void extractRealData(const std::string& db_path,
                     std::vector<KernelDispatch>& dispatches,
                     RefData& refs,
                     DispatchExtractOrder dispatch_order) {
    dispatches.clear();
    refs = RefData{};

    sqlite3* db = nullptr;
    if (sqlite3_open_v2(db_path.c_str(), &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
        fprintf(stderr, "Failed to open %s\n", db_path.c_str());
        return;
    }

    const char* order_clause = "";
    const char* order_label = "";
    switch (dispatch_order) {
        case DispatchExtractOrder::ByStart:
            order_clause = " ORDER BY start";
            order_label = "ORDER BY start";
            break;
        case DispatchExtractOrder::ById:
            order_clause = " ORDER BY id";
            order_label = "ORDER BY id";
            break;
        case DispatchExtractOrder::Unordered:
            order_clause = "";
            order_label = "no ORDER BY";
            break;
    }

    sqlite3_stmt* stmt;
    char sql[1536];

    // Extract node
    snprintf(sql, sizeof(sql),
             "SELECT hash, machine_id FROM rocpd_info_node_%s LIMIT 1", GUID);
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        refs.node_hash = sqlite3_column_int64(stmt, 0);
        refs.machine_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    }
    sqlite3_finalize(stmt);

    // Extract process
    snprintf(sql, sizeof(sql), "SELECT pid FROM rocpd_info_process_%s LIMIT 1", GUID);
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        refs.process_pid = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    // Extract threads
    snprintf(sql, sizeof(sql), "SELECT id, tid FROM rocpd_info_thread_%s", GUID);
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        int tid = sqlite3_column_int(stmt, 1);
        refs.threads[id] = tid;
    }
    sqlite3_finalize(stmt);

    // Extract agents
    snprintf(sql, sizeof(sql), "SELECT id, type, name FROM rocpd_info_agent_%s", GUID);
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char* type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        refs.agents[id] = {type ? type : "GPU", name ? name : ""};
    }
    sqlite3_finalize(stmt);

    // Extract queues
    snprintf(sql, sizeof(sql), "SELECT id, name FROM rocpd_info_queue_%s", GUID);
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        refs.queues[id] = name ? name : "";
    }
    sqlite3_finalize(stmt);

    // Extract streams
    snprintf(sql, sizeof(sql), "SELECT id, name FROM rocpd_info_stream_%s", GUID);
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        refs.streams[id] = name ? name : "";
    }
    sqlite3_finalize(stmt);

    // Extract code objects
    snprintf(sql, sizeof(sql), "SELECT id, uri FROM rocpd_info_code_object_%s", GUID);
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char* uri = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        refs.code_objects[id] = uri ? uri : "";
    }
    sqlite3_finalize(stmt);

    // Extract kernels
    snprintf(sql, sizeof(sql),
             "SELECT id, code_object_id, kernel_name FROM rocpd_info_kernel_symbol_%s",
             GUID);
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        int co_id = sqlite3_column_int(stmt, 1);
        const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        refs.kernels[id] = {co_id, name ? name : ""};
    }
    sqlite3_finalize(stmt);

    snprintf(sql, sizeof(sql),
             "SELECT nid, pid, tid, agent_id, kernel_id, dispatch_id, queue_id, "
             "stream_id, "
             "start, \"end\", workgroup_size_x, workgroup_size_y, workgroup_size_z, "
             "grid_size_x, grid_size_y, grid_size_z "
             "FROM rocpd_kernel_dispatch_%s%s",
             GUID, order_clause);
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        KernelDispatch d;
        d.nid = sqlite3_column_int(stmt, 0);
        d.pid = sqlite3_column_int(stmt, 1);
        d.tid = sqlite3_column_int(stmt, 2);
        d.agent_id = sqlite3_column_int(stmt, 3);
        d.kernel_id = sqlite3_column_int(stmt, 4);
        d.dispatch_id = sqlite3_column_int(stmt, 5);
        d.queue_id = sqlite3_column_int(stmt, 6);
        d.stream_id = sqlite3_column_int(stmt, 7);
        d.start = sqlite3_column_int64(stmt, 8);
        d.end = sqlite3_column_int64(stmt, 9);
        d.workgroup_size_x = sqlite3_column_int(stmt, 10);
        d.workgroup_size_y = sqlite3_column_int(stmt, 11);
        d.workgroup_size_z = sqlite3_column_int(stmt, 12);
        d.grid_size_x = sqlite3_column_int(stmt, 13);
        d.grid_size_y = sqlite3_column_int(stmt, 14);
        d.grid_size_z = sqlite3_column_int(stmt, 15);
        dispatches.push_back(d);
    }
    sqlite3_finalize(stmt);

    sqlite3_close(db);

    printf("Extracted (%s): %zu dispatches, %zu threads, %zu agents, %zu queues, %zu "
           "streams, %zu kernels\n",
           order_label, dispatches.size(), refs.threads.size(), refs.agents.size(),
           refs.queues.size(), refs.streams.size(), refs.kernels.size());
}
