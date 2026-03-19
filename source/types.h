#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

// Suffix for real database tables
extern const char* GUID;

// Kernel dispatch event data
struct KernelDispatch {
    int nid;
    int pid;
    int tid;
    int agent_id;
    int kernel_id;
    int dispatch_id;
    int queue_id;
    int stream_id;
    int64_t start;
    int64_t end;
    int workgroup_size_x;
    int workgroup_size_y;
    int workgroup_size_z;
    int grid_size_x;
    int grid_size_y;
    int grid_size_z;
};

// Reference data extracted from database
struct RefData {
    // Node
    int64_t node_hash;
    std::string machine_id;

    // Process
    int process_pid;

    // Threads (id -> tid value)
    std::unordered_map<int, int> threads;

    // Agents (id -> type, name)
    std::unordered_map<int, std::pair<std::string, std::string>> agents;

    // Queues (id -> name)
    std::unordered_map<int, std::string> queues;

    // Streams (id -> name)
    std::unordered_map<int, std::string> streams;

    // Code objects (id -> uri)
    std::unordered_map<int, std::string> code_objects;

    // Kernels (id -> code_object_id, name)
    std::unordered_map<int, std::pair<int, std::string>> kernels;
};
