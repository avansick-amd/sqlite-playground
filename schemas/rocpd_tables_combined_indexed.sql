-- Combined ROCpd schema WITH ADDITIONAL INDEXES
-- Combines tid, agent_id, nid, pid, queue_id, stream_id into a single context table
-- Reduces FK lookups from 7 to 2 (context + kernel)
-- ADDS strategic indexes for faster JOINs

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

-- Combined execution context table
-- Each unique combination of (nid, pid, tid, agent_id, queue_id, stream_id) gets one row
CREATE TABLE IF NOT EXISTS
    "rocpd_dispatch_context" (
        "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        "nid" INTEGER NOT NULL,
        "pid" INTEGER NOT NULL,
        "tid" INTEGER NOT NULL,
        "agent_id" INTEGER NOT NULL,
        "agent_type" TEXT CHECK ("agent_type" IN ('CPU', 'GPU')),
        "agent_name" TEXT,
        "queue_id" INTEGER NOT NULL,
        "queue_name" TEXT,
        "stream_id" INTEGER NOT NULL,
        "stream_name" TEXT,
        FOREIGN KEY (nid) REFERENCES "rocpd_info_node" (id) ON DELETE CASCADE,
        FOREIGN KEY (pid) REFERENCES "rocpd_info_process" (id) ON DELETE CASCADE
    );

-- Unique constraint to deduplicate contexts
CREATE UNIQUE INDEX IF NOT EXISTS idx_context_unique
    ON rocpd_dispatch_context(nid, pid, tid, agent_id, queue_id, stream_id);

-- Simplified kernel_dispatch with only 2 FKs: context + kernel
CREATE TABLE IF NOT EXISTS
    "rocpd_kernel_dispatch" (
        "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        "context_id" INTEGER NOT NULL,
        "kernel_id" INTEGER NOT NULL,
        "dispatch_id" INTEGER NOT NULL,
        "start" BIGINT NOT NULL,
        "end" BIGINT NOT NULL,
        "workgroup_size_x" INTEGER NOT NULL,
        "workgroup_size_y" INTEGER NOT NULL,
        "workgroup_size_z" INTEGER NOT NULL,
        "grid_size_x" INTEGER NOT NULL,
        "grid_size_y" INTEGER NOT NULL,
        "grid_size_z" INTEGER NOT NULL,
        FOREIGN KEY (context_id) REFERENCES "rocpd_dispatch_context" (id) ON DELETE CASCADE,
        FOREIGN KEY (kernel_id) REFERENCES "rocpd_info_kernel_symbol" (id) ON DELETE CASCADE
    );

-- ============ BASELINE INDEX ============
-- Index for common query patterns (ORDER BY start, time-range queries)
CREATE INDEX IF NOT EXISTS idx_dispatch_start ON rocpd_kernel_dispatch(start);

-- ============ ADDITIONAL INDEXES FOR PERFORMANCE ============

-- Foreign key indexes - speed up JOINs
CREATE INDEX IF NOT EXISTS idx_dispatch_context ON rocpd_kernel_dispatch(context_id);
CREATE INDEX IF NOT EXISTS idx_dispatch_kernel ON rocpd_kernel_dispatch(kernel_id);

-- Composite index for time-range + kernel filtering
CREATE INDEX IF NOT EXISTS idx_dispatch_start_kernel ON rocpd_kernel_dispatch(start, kernel_id);

-- Context component indexes for filtering
CREATE INDEX IF NOT EXISTS idx_context_tid ON rocpd_dispatch_context(tid);
CREATE INDEX IF NOT EXISTS idx_context_agent ON rocpd_dispatch_context(agent_id);
