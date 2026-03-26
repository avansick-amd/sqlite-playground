-- ============ ADDITIONAL INDEXES FOR PERFORMANCE ============

-- Foreign key indexes - speed up JOINs
CREATE INDEX IF NOT EXISTS idx_dispatch_context ON rocpd_kernel_dispatch(context_id);
CREATE INDEX IF NOT EXISTS idx_dispatch_kernel ON rocpd_kernel_dispatch(kernel_id);

-- Composite index for time-range + kernel filtering
CREATE INDEX IF NOT EXISTS idx_dispatch_start_kernel ON rocpd_kernel_dispatch(start, kernel_id);

-- Context component indexes for filtering
CREATE INDEX IF NOT EXISTS idx_context_tid ON rocpd_dispatch_context(tid);
CREATE INDEX IF NOT EXISTS idx_context_agent ON rocpd_dispatch_context(agent_id);
