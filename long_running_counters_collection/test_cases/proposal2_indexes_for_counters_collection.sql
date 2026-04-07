-- Source DB: ../bench_copies/02_proposal2_event_pmc_dispatch.db
-- Query: EXPLAIN QUERY PLAN SELECT * FROM counters_collection;
--
-- Named indexes referenced by the planner (non-autoindex):
-- idx_rocpd_kernel_dispatch_event_dispatch_agent, idx_rocpd_pmc_event_event_pmc
--
-- Regenerate: python3 test_cases/extract_counters_collection_plan_indexes.py


CREATE INDEX idx_rocpd_kernel_dispatch_event_dispatch_agent
ON rocpd_kernel_dispatch_00002e0d_9533_7533_b0f4_7d200442d314 (event_id, dispatch_id, agent_id);

CREATE INDEX idx_rocpd_pmc_event_event_pmc
ON rocpd_pmc_event_00002e0d_9533_7533_b0f4_7d200442d314 (event_id, pmc_id);
