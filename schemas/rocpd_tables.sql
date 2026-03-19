-- Original ROCpd schema (benchmark subset)
-- Has redundant nid, pid columns for denormalized access

PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS
    "rocpd_info_node" (
        "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        "hash" BIGINT NOT NULL UNIQUE,
        "machine_id" TEXT NOT NULL UNIQUE
    );

CREATE TABLE IF NOT EXISTS
    "rocpd_info_process" (
        "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        "nid" INTEGER NOT NULL,
        "pid" INTEGER NOT NULL,
        FOREIGN KEY (nid) REFERENCES "rocpd_info_node" (id) ON DELETE CASCADE
    );

CREATE TABLE IF NOT EXISTS
    "rocpd_info_thread" (
        "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        "nid" INTEGER NOT NULL,
        "pid" INTEGER NOT NULL,
        "tid" INTEGER NOT NULL,
        FOREIGN KEY (nid) REFERENCES "rocpd_info_node" (id) ON DELETE CASCADE,
        FOREIGN KEY (pid) REFERENCES "rocpd_info_process" (id) ON DELETE CASCADE
    );

CREATE TABLE IF NOT EXISTS
    "rocpd_info_agent" (
        "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        "nid" INTEGER NOT NULL,
        "pid" INTEGER NOT NULL,
        "type" TEXT CHECK ("type" IN ('CPU', 'GPU')),
        "name" TEXT,
        FOREIGN KEY (nid) REFERENCES "rocpd_info_node" (id) ON DELETE CASCADE,
        FOREIGN KEY (pid) REFERENCES "rocpd_info_process" (id) ON DELETE CASCADE
    );

CREATE TABLE IF NOT EXISTS
    "rocpd_info_queue" (
        "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        "nid" INTEGER NOT NULL,
        "pid" INTEGER NOT NULL,
        "name" TEXT,
        FOREIGN KEY (nid) REFERENCES "rocpd_info_node" (id) ON DELETE CASCADE,
        FOREIGN KEY (pid) REFERENCES "rocpd_info_process" (id) ON DELETE CASCADE
    );

CREATE TABLE IF NOT EXISTS
    "rocpd_info_stream" (
        "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        "nid" INTEGER NOT NULL,
        "pid" INTEGER NOT NULL,
        "name" TEXT,
        FOREIGN KEY (nid) REFERENCES "rocpd_info_node" (id) ON DELETE CASCADE,
        FOREIGN KEY (pid) REFERENCES "rocpd_info_process" (id) ON DELETE CASCADE
    );

CREATE TABLE IF NOT EXISTS
    "rocpd_info_code_object" (
        "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        "nid" INTEGER NOT NULL,
        "pid" INTEGER NOT NULL,
        "uri" TEXT,
        FOREIGN KEY (nid) REFERENCES "rocpd_info_node" (id) ON DELETE CASCADE,
        FOREIGN KEY (pid) REFERENCES "rocpd_info_process" (id) ON DELETE CASCADE
    );

CREATE TABLE IF NOT EXISTS
    "rocpd_info_kernel_symbol" (
        "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        "nid" INTEGER NOT NULL,
        "pid" INTEGER NOT NULL,
        "code_object_id" INTEGER NOT NULL,
        "kernel_name" TEXT,
        FOREIGN KEY (nid) REFERENCES "rocpd_info_node" (id) ON DELETE CASCADE,
        FOREIGN KEY (pid) REFERENCES "rocpd_info_process" (id) ON DELETE CASCADE,
        FOREIGN KEY (code_object_id) REFERENCES "rocpd_info_code_object" (id) ON DELETE CASCADE
    );

-- kernel_dispatch with redundant nid, pid for direct access (no JOINs needed)
CREATE TABLE IF NOT EXISTS
    "rocpd_kernel_dispatch" (
        "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        "nid" INTEGER NOT NULL,
        "pid" INTEGER NOT NULL,
        "tid" INTEGER,
        "agent_id" INTEGER NOT NULL,
        "kernel_id" INTEGER NOT NULL,
        "dispatch_id" INTEGER NOT NULL,
        "queue_id" INTEGER NOT NULL,
        "stream_id" INTEGER NOT NULL,
        "start" BIGINT NOT NULL,
        "end" BIGINT NOT NULL,
        "workgroup_size_x" INTEGER NOT NULL,
        "workgroup_size_y" INTEGER NOT NULL,
        "workgroup_size_z" INTEGER NOT NULL,
        "grid_size_x" INTEGER NOT NULL,
        "grid_size_y" INTEGER NOT NULL,
        "grid_size_z" INTEGER NOT NULL,
        FOREIGN KEY (nid) REFERENCES "rocpd_info_node" (id) ON DELETE CASCADE,
        FOREIGN KEY (pid) REFERENCES "rocpd_info_process" (id) ON DELETE CASCADE,
        FOREIGN KEY (tid) REFERENCES "rocpd_info_thread" (id) ON DELETE CASCADE,
        FOREIGN KEY (agent_id) REFERENCES "rocpd_info_agent" (id) ON DELETE CASCADE,
        FOREIGN KEY (kernel_id) REFERENCES "rocpd_info_kernel_symbol" (id) ON DELETE CASCADE,
        FOREIGN KEY (queue_id) REFERENCES "rocpd_info_queue" (id) ON DELETE CASCADE,
        FOREIGN KEY (stream_id) REFERENCES "rocpd_info_stream" (id) ON DELETE CASCADE
    );

-- Index for common query patterns (ORDER BY start, time-range queries)
CREATE INDEX IF NOT EXISTS idx_dispatch_start ON rocpd_kernel_dispatch(start);
