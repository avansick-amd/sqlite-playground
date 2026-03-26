-- ============ ADDITIONAL INDEXES FOR PERFORMANCE ============

-- Foreign key indexes - speed up JOINs
CREATE INDEX IF NOT EXISTS idx_dispatch_tid ON rocpd_kernel_dispatch(tid);
CREATE INDEX IF NOT EXISTS idx_dispatch_agent ON rocpd_kernel_dispatch(agent_id);
CREATE INDEX IF NOT EXISTS idx_dispatch_kernel ON rocpd_kernel_dispatch(kernel_id);
CREATE INDEX IF NOT EXISTS idx_dispatch_queue ON rocpd_kernel_dispatch(queue_id);
CREATE INDEX IF NOT EXISTS idx_dispatch_stream ON rocpd_kernel_dispatch(stream_id);

-- Composite index for time-range + kernel filtering
CREATE INDEX IF NOT EXISTS idx_dispatch_start_kernel ON rocpd_kernel_dispatch(start, kernel_id);

-- Composite index for per-thread analysis
CREATE INDEX IF NOT EXISTS idx_dispatch_tid_start ON rocpd_kernel_dispatch(tid, start);
