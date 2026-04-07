-- Extracted schema from profiler capture (read-only introspection).
-- Source file: benchmark_data/rocpd-7389.db
-- Generated for reference; not used as the playground DDL.
-- Table names include a per-capture GUID suffix in the live DB.

CREATE TABLE "rocpd_metadata_eb95343b77a98cb86f9e54335edbb3f4" (
        "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        "tag" TEXT NOT NULL,
        "value" TEXT NOT NULL
    );

CREATE TABLE sqlite_sequence(name,seq);

CREATE TABLE `rocpd_string_eb95343b77a98cb86f9e54335edbb3f4` (
        "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        "guid" TEXT DEFAULT "eb95343b77a98cb86f9e54335edbb3f4" NOT NULL,
        "string" TEXT NOT NULL UNIQUE ON CONFLICT ABORT
    );

CREATE TABLE `rocpd_info_node_eb95343b77a98cb86f9e54335edbb3f4` (
        "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        "guid" TEXT DEFAULT "eb95343b77a98cb86f9e54335edbb3f4" NOT NULL,
        "hash" BIGINT NOT NULL UNIQUE,
        "machine_id" TEXT NOT NULL UNIQUE,
        "system_name" TEXT,
        "hostname" TEXT,
        "release" TEXT,
        "version" TEXT,
        "hardware_name" TEXT,
        "domain_name" TEXT
    );

CREATE TABLE `rocpd_info_process_eb95343b77a98cb86f9e54335edbb3f4` (
        "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        "guid" TEXT DEFAULT "eb95343b77a98cb86f9e54335edbb3f4" NOT NULL,
        "nid" INTEGER NOT NULL,
        "ppid" INTEGER,
        "pid" INTEGER NOT NULL,
        "init" BIGINT,
        "fini" BIGINT,
        "start" BIGINT,
        "end" BIGINT,
        "command" TEXT,
        "environment" JSONB DEFAULT "{}" NOT NULL,
        "extdata" JSONB DEFAULT "{}" NOT NULL,
        FOREIGN KEY (nid) REFERENCES `rocpd_info_node_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE
    );

CREATE TABLE `rocpd_info_thread_eb95343b77a98cb86f9e54335edbb3f4` (
        "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        "guid" TEXT DEFAULT "eb95343b77a98cb86f9e54335edbb3f4" NOT NULL,
        "nid" INTEGER NOT NULL,
        "ppid" INTEGER,
        "pid" INTEGER NOT NULL,
        "tid" INTEGER NOT NULL,
        "name" TEXT,
        "start" BIGINT,
        "end" BIGINT,
        "extdata" JSONB DEFAULT "{}" NOT NULL,
        FOREIGN KEY (nid) REFERENCES `rocpd_info_node_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (pid) REFERENCES `rocpd_info_process_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE
    );

CREATE TABLE `rocpd_info_agent_eb95343b77a98cb86f9e54335edbb3f4` (
        "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        "guid" TEXT DEFAULT "eb95343b77a98cb86f9e54335edbb3f4" NOT NULL,
        "nid" INTEGER NOT NULL,
        "pid" INTEGER NOT NULL,
        "type" TEXT CHECK ("type" IN ('CPU', 'GPU')),
        "absolute_index" INTEGER,
        "logical_index" INTEGER,
        "type_index" INTEGER,
        "uuid" INTEGER,
        "name" TEXT,
        "model_name" TEXT,
        "vendor_name" TEXT,
        "product_name" TEXT,
        "user_name" TEXT,
        "extdata" JSONB DEFAULT "{}" NOT NULL,
        FOREIGN KEY (nid) REFERENCES `rocpd_info_node_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (pid) REFERENCES `rocpd_info_process_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE
    );

CREATE TABLE `rocpd_info_queue_eb95343b77a98cb86f9e54335edbb3f4` (
        "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        "guid" TEXT DEFAULT "eb95343b77a98cb86f9e54335edbb3f4" NOT NULL,
        "nid" INTEGER NOT NULL,
        "pid" INTEGER NOT NULL,
        "name" TEXT,
        "extdata" JSONB DEFAULT "{}" NOT NULL,
        FOREIGN KEY (nid) REFERENCES `rocpd_info_node_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (pid) REFERENCES `rocpd_info_process_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE
    );

CREATE TABLE `rocpd_info_stream_eb95343b77a98cb86f9e54335edbb3f4` (
        "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        "guid" TEXT DEFAULT "eb95343b77a98cb86f9e54335edbb3f4" NOT NULL,
        "nid" INTEGER NOT NULL,
        "pid" INTEGER NOT NULL,
        "name" TEXT,
        "extdata" JSONB DEFAULT "{}" NOT NULL,
        FOREIGN KEY (nid) REFERENCES `rocpd_info_node_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (pid) REFERENCES `rocpd_info_process_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE
    );

CREATE TABLE `rocpd_info_pmc_eb95343b77a98cb86f9e54335edbb3f4` (
        "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        "guid" TEXT DEFAULT "eb95343b77a98cb86f9e54335edbb3f4" NOT NULL,
        "nid" INTEGER NOT NULL,
        "pid" INTEGER NOT NULL,
        "agent_id" INTEGER,
        "target_arch" TEXT CHECK ("target_arch" IN ('CPU', 'GPU')),
        "event_code" INT,
        "instance_id" INTEGER,
        "name" TEXT NOT NULL,
        "symbol" TEXT NOT NULL,
        "description" TEXT,
        "long_description" TEXT DEFAULT "",
        "component" TEXT,
        "units" TEXT DEFAULT "",
        "value_type" TEXT CHECK ("value_type" IN ('ABS', 'ACCUM', 'RELATIVE')),
        "block" TEXT,
        "expression" TEXT,
        "is_constant" INTEGER,
        "is_derived" INTEGER,
        "extdata" JSONB DEFAULT "{}" NOT NULL,
        FOREIGN KEY (nid) REFERENCES `rocpd_info_node_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (pid) REFERENCES `rocpd_info_process_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (agent_id) REFERENCES `rocpd_info_agent_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE
    );

CREATE TABLE `rocpd_info_code_object_eb95343b77a98cb86f9e54335edbb3f4` (
        "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        "guid" TEXT DEFAULT "eb95343b77a98cb86f9e54335edbb3f4" NOT NULL,
        "nid" INTEGER NOT NULL,
        "pid" INTEGER NOT NULL,
        "agent_id" INTEGER,
        "uri" TEXT,
        "load_base" BIGINT,
        "load_size" BIGINT,
        "load_delta" BIGINT,
        "storage_type" TEXT CHECK ("storage_type" IN ('FILE', 'MEMORY')),
        "extdata" JSONB DEFAULT "{}" NOT NULL,
        FOREIGN KEY (nid) REFERENCES `rocpd_info_node_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (pid) REFERENCES `rocpd_info_process_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (agent_id) REFERENCES `rocpd_info_agent_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE
    );

CREATE TABLE `rocpd_info_kernel_symbol_eb95343b77a98cb86f9e54335edbb3f4` (
        "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        "guid" TEXT DEFAULT "eb95343b77a98cb86f9e54335edbb3f4" NOT NULL,
        "nid" INTEGER NOT NULL,
        "pid" INTEGER NOT NULL,
        "code_object_id" INTEGER NOT NULL,
        "kernel_name" TEXT,
        "display_name" TEXT,
        "kernel_object" INTEGER,
        "kernarg_segment_size" INTEGER,
        "kernarg_segment_alignment" INTEGER,
        "group_segment_size" INTEGER,
        "private_segment_size" INTEGER,
        "sgpr_count" INTEGER,
        "arch_vgpr_count" INTEGER,
        "accum_vgpr_count" INTEGER,
        "extdata" JSONB DEFAULT "{}" NOT NULL,
        FOREIGN KEY (nid) REFERENCES `rocpd_info_node_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (pid) REFERENCES `rocpd_info_process_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (code_object_id) REFERENCES `rocpd_info_code_object_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE
    );

CREATE TABLE `rocpd_track_eb95343b77a98cb86f9e54335edbb3f4` (
        "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        "guid" TEXT DEFAULT "eb95343b77a98cb86f9e54335edbb3f4" NOT NULL,
        "nid" INTEGER NOT NULL,
        "pid" INTEGER,
        "tid" INTEGER,
        "name_id" INTEGER,
        "extdata" JSONB DEFAULT "{}" NOT NULL,
        FOREIGN KEY (nid) REFERENCES `rocpd_info_node_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (pid) REFERENCES `rocpd_info_process_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (tid) REFERENCES `rocpd_info_thread_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (name_id) REFERENCES `rocpd_string_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE
    );

CREATE TABLE `rocpd_event_eb95343b77a98cb86f9e54335edbb3f4` (
        "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        "guid" TEXT DEFAULT "eb95343b77a98cb86f9e54335edbb3f4" NOT NULL,
        "category_id" INTEGER,
        "stack_id" INTEGER,
        "parent_stack_id" INTEGER,
        "correlation_id" INTEGER,
        "call_stack" JSONB DEFAULT "{}" NOT NULL,
        "line_info" JSONB DEFAULT "{}" NOT NULL,
        "extdata" JSONB DEFAULT "{}" NOT NULL,
        FOREIGN KEY (category_id) REFERENCES `rocpd_string_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE
    );

CREATE TABLE `rocpd_arg_eb95343b77a98cb86f9e54335edbb3f4` (
        "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        "guid" TEXT DEFAULT "eb95343b77a98cb86f9e54335edbb3f4" NOT NULL,
        "event_id" INTEGER NOT NULL,
        "position" INTEGER NOT NULL,
        "type" TEXT NOT NULL,
        "name" TEXT NOT NULL,
        "value" TEXT, -- TODO: discuss make it value_id and integer, refer to string table --
        "extdata" JSONB DEFAULT "{}" NOT NULL,
        FOREIGN KEY (event_id) REFERENCES `rocpd_event_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE
    );

CREATE TABLE `rocpd_pmc_event_eb95343b77a98cb86f9e54335edbb3f4` (
        "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        "guid" TEXT DEFAULT "eb95343b77a98cb86f9e54335edbb3f4" NOT NULL,
        "event_id" INTEGER,
        "pmc_id" INTEGER NOT NULL,
        "value" REAL DEFAULT 0.0,
        "extdata" JSONB DEFAULT "{}",
        FOREIGN KEY (pmc_id) REFERENCES `rocpd_info_pmc_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (event_id) REFERENCES `rocpd_event_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE
    );

CREATE TABLE `rocpd_region_eb95343b77a98cb86f9e54335edbb3f4` (
        "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        "guid" TEXT DEFAULT "eb95343b77a98cb86f9e54335edbb3f4" NOT NULL,
        "nid" INTEGER NOT NULL,
        "pid" INTEGER NOT NULL,
        "tid" INTEGER NOT NULL,
        "start" BIGINT NOT NULL,
        "end" BIGINT NOT NULL,
        "name_id" INTEGER NOT NULL,
        "event_id" INTEGER,
        "extdata" JSONB DEFAULT "{}" NOT NULL,
        FOREIGN KEY (nid) REFERENCES `rocpd_info_node_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (pid) REFERENCES `rocpd_info_process_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (tid) REFERENCES `rocpd_info_thread_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (name_id) REFERENCES `rocpd_string_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (event_id) REFERENCES `rocpd_event_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE
    );

CREATE TABLE `rocpd_sample_eb95343b77a98cb86f9e54335edbb3f4` (
        "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        "guid" TEXT DEFAULT "eb95343b77a98cb86f9e54335edbb3f4" NOT NULL,
        "track_id" INTEGER NOT NULL,
        "timestamp" BIGINT NOT NULL,
        "event_id" INTEGER,
        "extdata" JSONB DEFAULT "{}" NOT NULL,
        FOREIGN KEY (track_id) REFERENCES `rocpd_track_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (event_id) REFERENCES `rocpd_event_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE
    );

CREATE TABLE `rocpd_kernel_dispatch_eb95343b77a98cb86f9e54335edbb3f4` (
        "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        "guid" TEXT DEFAULT "eb95343b77a98cb86f9e54335edbb3f4" NOT NULL,
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
        "private_segment_size" INTEGER,
        "group_segment_size" INTEGER,
        "workgroup_size_x" INTEGER NOT NULL,
        "workgroup_size_y" INTEGER NOT NULL,
        "workgroup_size_z" INTEGER NOT NULL,
        "grid_size_x" INTEGER NOT NULL,
        "grid_size_y" INTEGER NOT NULL,
        "grid_size_z" INTEGER NOT NULL,
        "region_name_id" INTEGER,
        "event_id" INTEGER,
        "extdata" JSONB DEFAULT "{}" NOT NULL,
        FOREIGN KEY (nid) REFERENCES `rocpd_info_node_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (pid) REFERENCES `rocpd_info_process_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (tid) REFERENCES `rocpd_info_thread_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (agent_id) REFERENCES `rocpd_info_agent_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (kernel_id) REFERENCES `rocpd_info_kernel_symbol_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (queue_id) REFERENCES `rocpd_info_queue_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (stream_id) REFERENCES `rocpd_info_stream_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (region_name_id) REFERENCES `rocpd_string_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (event_id) REFERENCES `rocpd_event_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE
    );

CREATE TABLE `rocpd_memory_copy_eb95343b77a98cb86f9e54335edbb3f4` (
        "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        "guid" TEXT DEFAULT "eb95343b77a98cb86f9e54335edbb3f4" NOT NULL,
        "nid" INTEGER NOT NULL,
        "pid" INTEGER NOT NULL,
        "tid" INTEGER,
        "start" BIGINT NOT NULL,
        "end" BIGINT NOT NULL,
        "name_id" INTEGER NOT NULL,
        "dst_agent_id" INTEGER,
        "dst_address" INTEGER,
        "src_agent_id" INTEGER,
        "src_address" INTEGER,
        "size" INTEGER NOT NULL,
        "queue_id" INTEGER,
        "stream_id" INTEGER,
        "region_name_id" INTEGER,
        "event_id" INTEGER,
        "extdata" JSONB DEFAULT "{}" NOT NULL,
        FOREIGN KEY (nid) REFERENCES `rocpd_info_node_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (pid) REFERENCES `rocpd_info_process_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (tid) REFERENCES `rocpd_info_thread_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (name_id) REFERENCES `rocpd_string_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (dst_agent_id) REFERENCES `rocpd_info_agent_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (src_agent_id) REFERENCES `rocpd_info_agent_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (stream_id) REFERENCES `rocpd_info_stream_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (queue_id) REFERENCES `rocpd_info_queue_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (region_name_id) REFERENCES `rocpd_string_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (event_id) REFERENCES `rocpd_event_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE
    );

CREATE TABLE `rocpd_memory_allocate_eb95343b77a98cb86f9e54335edbb3f4` (
        "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        "guid" TEXT DEFAULT "eb95343b77a98cb86f9e54335edbb3f4" NOT NULL,
        "nid" INTEGER NOT NULL,
        "pid" INTEGER NOT NULL,
        "tid" INTEGER,
        "agent_id" INTEGER,
        "type" TEXT CHECK ("type" IN ('ALLOC', 'FREE', 'REALLOC', 'RECLAIM')),
        "level" TEXT CHECK ("level" IN ('REAL', 'VIRTUAL', 'SCRATCH')),
        "start" BIGINT NOT NULL,
        "end" BIGINT NOT NULL,
        "address" INTEGER,
        "size" INTEGER NOT NULL,
        "queue_id" INTEGER,
        "stream_id" INTEGER,
        "event_id" INTEGER,
        "extdata" JSONB DEFAULT "{}" NOT NULL,
        FOREIGN KEY (nid) REFERENCES `rocpd_info_node_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (pid) REFERENCES `rocpd_info_process_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (tid) REFERENCES `rocpd_info_thread_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (agent_id) REFERENCES `rocpd_info_agent_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (stream_id) REFERENCES `rocpd_info_stream_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (queue_id) REFERENCES `rocpd_info_queue_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE,
        FOREIGN KEY (event_id) REFERENCES `rocpd_event_eb95343b77a98cb86f9e54335edbb3f4` (id) ON UPDATE CASCADE
    );

CREATE VIEW `rocpd_metadata` AS
SELECT
    *
FROM
    `rocpd_metadata_eb95343b77a98cb86f9e54335edbb3f4`;

CREATE VIEW `rocpd_string` AS
SELECT
    *
FROM
    `rocpd_string_eb95343b77a98cb86f9e54335edbb3f4`;

CREATE VIEW `rocpd_info_node` AS
SELECT
    *
FROM
    `rocpd_info_node_eb95343b77a98cb86f9e54335edbb3f4`;

CREATE VIEW `rocpd_info_process` AS
SELECT
    *
FROM
    `rocpd_info_process_eb95343b77a98cb86f9e54335edbb3f4`;

CREATE VIEW `rocpd_info_thread` AS
SELECT
    *
FROM
    `rocpd_info_thread_eb95343b77a98cb86f9e54335edbb3f4`;

CREATE VIEW `rocpd_info_agent` AS
SELECT
    *
FROM
    `rocpd_info_agent_eb95343b77a98cb86f9e54335edbb3f4`;

CREATE VIEW `rocpd_info_queue` AS
SELECT
    *
FROM
    `rocpd_info_queue_eb95343b77a98cb86f9e54335edbb3f4`;

CREATE VIEW `rocpd_info_stream` AS
SELECT
    *
FROM
    `rocpd_info_stream_eb95343b77a98cb86f9e54335edbb3f4`;

CREATE VIEW `rocpd_info_pmc` AS
SELECT
    *
FROM
    `rocpd_info_pmc_eb95343b77a98cb86f9e54335edbb3f4`;

CREATE VIEW `rocpd_info_code_object` AS
SELECT
    *
FROM
    `rocpd_info_code_object_eb95343b77a98cb86f9e54335edbb3f4`;

CREATE VIEW `rocpd_info_kernel_symbol` AS
SELECT
    *
FROM
    `rocpd_info_kernel_symbol_eb95343b77a98cb86f9e54335edbb3f4`;

CREATE VIEW `rocpd_track` AS
SELECT
    *
FROM
    `rocpd_track_eb95343b77a98cb86f9e54335edbb3f4`;

CREATE VIEW `rocpd_event` AS
SELECT
    *
FROM
    `rocpd_event_eb95343b77a98cb86f9e54335edbb3f4`;

CREATE VIEW `rocpd_arg` AS
SELECT
    *
FROM
    `rocpd_arg_eb95343b77a98cb86f9e54335edbb3f4`;

CREATE VIEW `rocpd_pmc_event` AS
SELECT
    *
FROM
    `rocpd_pmc_event_eb95343b77a98cb86f9e54335edbb3f4`;

CREATE VIEW `rocpd_region` AS
SELECT
    *
FROM
    `rocpd_region_eb95343b77a98cb86f9e54335edbb3f4`;

CREATE VIEW `rocpd_sample` AS
SELECT
    *
FROM
    `rocpd_sample_eb95343b77a98cb86f9e54335edbb3f4`;

CREATE VIEW `rocpd_kernel_dispatch` AS
SELECT
    *
FROM
    `rocpd_kernel_dispatch_eb95343b77a98cb86f9e54335edbb3f4`;

CREATE VIEW `rocpd_memory_copy` AS
SELECT
    *
FROM
    `rocpd_memory_copy_eb95343b77a98cb86f9e54335edbb3f4`;

CREATE VIEW `rocpd_memory_allocate` AS
SELECT
    *
FROM
    `rocpd_memory_allocate_eb95343b77a98cb86f9e54335edbb3f4`;

CREATE VIEW `code_objects` AS
SELECT
    CO.id,
    CO.guid,
    CO.nid,
    P.pid,
    A.absolute_index AS agent_abs_index,
    CO.uri,
    CO.load_base,
    CO.load_size,
    CO.load_delta,
    CO.storage_type AS storage_type_str,
    JSON_EXTRACT(CO.extdata, '$.size') AS code_object_size,
    JSON_EXTRACT(CO.extdata, '$.storage_type') AS storage_type,
    JSON_EXTRACT(CO.extdata, '$.memory_base') AS memory_base,
    JSON_EXTRACT(CO.extdata, '$.memory_size') AS memory_size
FROM
    `rocpd_info_code_object` CO
    INNER JOIN `rocpd_info_agent` A ON CO.agent_id = A.id
    AND CO.guid = A.guid
    INNER JOIN `rocpd_info_process` P ON CO.pid = P.id
    AND CO.guid = P.guid;

CREATE VIEW `kernel_symbols` AS
SELECT
    KS.id,
    KS.guid,
    KS.nid,
    P.pid,
    KS.code_object_id,
    KS.kernel_name,
    KS.display_name,
    KS.kernel_object,
    KS.kernarg_segment_size,
    KS.kernarg_segment_alignment,
    KS.group_segment_size,
    KS.private_segment_size,
    KS.sgpr_count,
    KS.arch_vgpr_count,
    KS.accum_vgpr_count,
    JSON_EXTRACT(KS.extdata, '$.size') AS kernel_symbol_size,
    JSON_EXTRACT(KS.extdata, '$.kernel_id') AS kernel_id,
    JSON_EXTRACT(KS.extdata, '$.kernel_code_entry_byte_offset') AS kernel_code_entry_byte_offset,
    JSON_EXTRACT(KS.extdata, '$.formatted_kernel_name') AS formatted_kernel_name,
    JSON_EXTRACT(KS.extdata, '$.demangled_kernel_name') AS demangled_kernel_name,
    JSON_EXTRACT(KS.extdata, '$.truncated_kernel_name') AS truncated_kernel_name,
    JSON_EXTRACT(KS.extdata, '$.kernel_address.handle') AS kernel_address
FROM
    `rocpd_info_kernel_symbol` KS
    INNER JOIN `rocpd_info_process` P ON KS.pid = P.id
    AND KS.guid = P.guid;

CREATE VIEW `processes` AS
SELECT
    N.id AS nid,
    N.machine_id,
    N.system_name,
    N.hostname,
    N.release AS system_release,
    N.version AS system_version,
    P.guid,
    P.ppid,
    P.pid,
    P.init,
    P.start,
    P.end,
    P.fini,
    P.command
FROM
    `rocpd_info_process` P
    INNER JOIN `rocpd_info_node` N ON N.id = P.nid
    AND N.guid = P.guid;

CREATE VIEW `threads` AS
SELECT
    N.id AS nid,
    N.machine_id,
    N.system_name,
    N.hostname,
    N.release AS system_release,
    N.version AS system_version,
    P.guid,
    P.ppid,
    P.pid,
    T.tid,
    T.start,
    T.end,
    T.name
FROM
    `rocpd_info_thread` T
    INNER JOIN `rocpd_info_process` P ON P.id = T.pid
    AND N.guid = T.guid
    INNER JOIN `rocpd_info_node` N ON N.id = T.nid
    AND N.guid = T.guid;

CREATE VIEW `regions` AS
SELECT
    R.id,
    R.guid,
    (
        SELECT
            string
        FROM
            `rocpd_string` RS
        WHERE
            RS.id = E.category_id
            AND RS.guid = E.guid
    ) AS category,
    S.string AS name,
    R.nid,
    P.pid,
    T.tid,
    R.start,
    R.end,
    (R.end - R.start) AS duration,
    R.event_id,
    E.stack_id,
    E.parent_stack_id,
    E.correlation_id AS corr_id,
    E.extdata,
    E.call_stack,
    E.line_info
FROM
    `rocpd_region` R
    INNER JOIN `rocpd_event` E ON E.id = R.event_id
    AND E.guid = R.guid
    INNER JOIN `rocpd_string` S ON S.id = R.name_id
    AND S.guid = R.guid
    INNER JOIN `rocpd_info_process` P ON P.id = R.pid
    AND P.guid = R.guid
    INNER JOIN `rocpd_info_thread` T ON T.id = R.tid
    AND T.guid = R.guid;

CREATE VIEW `region_args` AS
SELECT
    R.id,
    R.guid,
    R.nid,
    P.pid,
    A.type,
    A.name,
    A.value
FROM
    `rocpd_region` R
    INNER JOIN `rocpd_event` E ON E.id = R.event_id
    AND E.guid = R.guid
    INNER JOIN `rocpd_arg` A ON A.event_id = E.id
    AND A.guid = R.guid
    INNER JOIN `rocpd_info_process` P ON P.id = R.pid
    AND P.guid = R.guid;

CREATE VIEW `samples` AS
SELECT
    R.id,
    R.guid,
    (
        SELECT
            string
        FROM
            `rocpd_string` RS
        WHERE
            RS.id = E.category_id
            AND RS.guid = E.guid
    ) AS category,
    (
        SELECT
            string
        FROM
            `rocpd_string` RS
        WHERE
            RS.id = T.name_id
            AND RS.guid = T.guid
    ) AS name,
    T.nid,
    P.pid,
    TH.tid,
    R.timestamp,
    R.event_id,
    E.stack_id AS stack_id,
    E.parent_stack_id AS parent_stack_id,
    E.correlation_id AS corr_id,
    E.extdata AS extdata,
    E.call_stack AS call_stack,
    E.line_info AS line_info
FROM
    `rocpd_sample` R
    INNER JOIN `rocpd_track` T ON T.id = R.track_id
    AND T.guid = R.guid
    INNER JOIN `rocpd_event` E ON E.id = R.event_id
    AND E.guid = R.guid
    INNER JOIN `rocpd_info_process` P ON P.id = T.pid
    AND P.guid = T.guid
    INNER JOIN `rocpd_info_thread` TH ON TH.id = T.tid
    AND TH.guid = T.guid;

CREATE VIEW `sample_regions` AS
SELECT
    R.id,
    R.guid,
    (
        SELECT
            string
        FROM
            `rocpd_string` RS
        WHERE
            RS.id = E.category_id
            AND RS.guid = E.guid
    ) AS category,
    (
        SELECT
            string
        FROM
            `rocpd_string` RS
        WHERE
            RS.id = T.name_id
            AND RS.guid = T.guid
    ) AS name,
    T.nid,
    P.pid,
    TH.tid,
    R.timestamp AS start,
    R.timestamp AS END,
    (R.timestamp - R.timestamp) AS duration,
    R.event_id,
    E.stack_id AS stack_id,
    E.parent_stack_id AS parent_stack_id,
    E.correlation_id AS corr_id,
    E.extdata AS extdata,
    E.call_stack AS call_stack,
    E.line_info AS line_info
FROM
    `rocpd_sample` R
    INNER JOIN `rocpd_track` T ON T.id = R.track_id
    AND T.guid = R.guid
    INNER JOIN `rocpd_event` E ON E.id = R.event_id
    AND E.guid = R.guid
    INNER JOIN `rocpd_info_process` P ON P.id = T.pid
    AND P.guid = T.guid
    INNER JOIN `rocpd_info_thread` TH ON TH.id = T.tid
    AND TH.guid = T.guid;

CREATE VIEW `regions_and_samples` AS
SELECT
    *
FROM
    `regions`
UNION ALL
SELECT
    *
FROM
    `sample_regions`;

CREATE VIEW `kernels` AS
SELECT
    K.id,
    K.guid,
    T.tid,
    (
        SELECT
            string
        FROM
            `rocpd_string` RS
        WHERE
            RS.id = E.category_id
            AND RS.guid = E.guid
    ) AS category,
    R.string AS region,
    S.display_name AS name,
    K.nid,
    P.pid,
    A.absolute_index AS agent_abs_index,
    A.logical_index AS agent_log_index,
    A.type_index AS agent_type_index,
    A.type AS agent_type,
    S.code_object_id AS code_object_id,
    K.kernel_id,
    K.dispatch_id,
    K.stream_id,
    K.queue_id,
    Q.name AS queue,
    ST.name AS stream,
    K.start,
    K.end,
    (K.end - K.start) AS duration,
    K.grid_size_x AS grid_x,
    K.grid_size_y AS grid_y,
    K.grid_size_z AS grid_z,
    K.workgroup_size_x AS workgroup_x,
    K.workgroup_size_y AS workgroup_y,
    K.workgroup_size_z AS workgroup_z,
    K.group_segment_size AS lds_size,
    K.private_segment_size AS scratch_size,
    S.group_segment_size AS static_lds_size,
    S.private_segment_size AS static_scratch_size,
    E.stack_id,
    E.parent_stack_id,
    E.correlation_id AS corr_id
FROM
    `rocpd_kernel_dispatch` K
    INNER JOIN `rocpd_info_agent` A ON A.id = K.agent_id
    AND A.guid = K.guid
    INNER JOIN `rocpd_event` E ON E.id = K.event_id
    AND E.guid = K.guid
    INNER JOIN `rocpd_string` R ON R.id = K.region_name_id
    AND R.guid = K.guid
    INNER JOIN `rocpd_info_kernel_symbol` S ON S.id = K.kernel_id
    AND S.guid = K.guid
    LEFT JOIN `rocpd_info_stream` ST ON ST.id = K.stream_id
    AND ST.guid = K.guid
    LEFT JOIN `rocpd_info_queue` Q ON Q.id = K.queue_id
    AND Q.guid = K.guid
    INNER JOIN `rocpd_info_process` P ON P.id = Q.pid
    AND P.guid = Q.guid
    INNER JOIN `rocpd_info_thread` T ON T.id = K.tid
    AND T.guid = K.guid;

CREATE VIEW `pmc_info` AS
SELECT
    PMC_I.id,
    PMC_I.guid,
    PMC_I.nid,
    P.pid,
    A.absolute_index AS agent_abs_index,
    PMC_I.is_constant,
    PMC_I.is_derived,
    PMC_I.name,
    PMC_I.description,
    PMC_I.block,
    PMC_I.expression
FROM
    `rocpd_info_pmc` PMC_I
    INNER JOIN `rocpd_info_agent` A ON PMC_I.agent_id = A.id
    AND PMC_I.guid = A.guid
    INNER JOIN `rocpd_info_process` P ON P.id = PMC_I.pid
    AND PMC_I.guid = P.guid;

CREATE VIEW `pmc_events` AS
SELECT
    PMC_E.id,
    PMC_E.guid,
    PMC_E.pmc_id,
    E.id AS event_id,
    (
        SELECT
            string
        FROM
            `rocpd_string` RS
        WHERE
            RS.id = E.category_id
            AND RS.guid = E.guid
    ) AS category,
    (
        SELECT
            display_name
        FROM
            `rocpd_info_kernel_symbol` KS
        WHERE
            KS.id = K.kernel_id
            AND KS.guid = K.guid
    ) AS name,
    K.nid,
    P.pid,
    K.dispatch_id,
    K.start,
    K.end,
    (K.end - K.start) AS duration,
    PMC_I.name AS counter_name,
    PMC_E.value AS counter_value
FROM
    `rocpd_pmc_event` PMC_E
    INNER JOIN `rocpd_info_pmc` PMC_I ON PMC_I.id = PMC_E.pmc_id
    AND PMC_I.guid = PMC_E.guid
    INNER JOIN `rocpd_event` E ON E.id = PMC_E.event_id
    AND E.guid = PMC_E.guid
    INNER JOIN `rocpd_kernel_dispatch` K ON K.event_id = PMC_E.event_id
    AND K.guid = PMC_E.guid
    INNER JOIN `rocpd_info_process` P ON P.id = K.pid
    AND P.guid = K.guid;

CREATE VIEW `events_args` AS
SELECT
    E.id AS event_id,
    (
        SELECT
            string
        FROM
            `rocpd_string` RS
        WHERE
            RS.id = E.category_id
            AND RS.guid = E.guid
    ) AS category,
    E.stack_id,
    E.parent_stack_id,
    E.correlation_id,
    A.position AS arg_position,
    A.type AS arg_type,
    A.name AS arg_name,
    A.value AS arg_value,
    E.call_stack,
    E.line_info,
    A.extdata
FROM
    `rocpd_event` E
    INNER JOIN `rocpd_arg` A ON A.event_id = E.id
    AND A.guid = E.guid;

CREATE VIEW `stream_args` AS
SELECT
    A.id AS argument_id,
    A.event_id AS event_id,
    A.position AS arg_position,
    A.type AS arg_type,
    A.value AS arg_value,
    JSON_EXTRACT(A.extdata, '$.stream_id') AS stream_id,
    S.nid,
    P.pid,
    S.name AS stream_name,
    S.extdata AS extdata
FROM
    `rocpd_arg` A
    INNER JOIN `rocpd_info_stream` S ON JSON_EXTRACT(A.extdata, '$.stream_id') = S.id
    AND A.guid = S.guid
    INNER JOIN `rocpd_info_process` P ON P.id = S.pid
    AND P.guid = S.guid
WHERE
    A.name = 'stream';

CREATE VIEW `memory_copies` AS
SELECT
    M.id,
    M.guid,
    (
        SELECT
            string
        FROM
            `rocpd_string` RS
        WHERE
            RS.id = E.category_id
            AND RS.guid = E.guid
    ) AS category,
    M.nid,
    P.pid,
    T.tid,
    M.start,
    M.end,
    (M.end - M.start) AS duration,
    S.string AS name,
    R.string AS region_name,
    M.stream_id,
    M.queue_id,
    ST.name AS stream_name,
    Q.name AS queue_name,
    M.size,
    dst_agent.name AS dst_device,
    dst_agent.absolute_index AS dst_agent_abs_index,
    dst_agent.logical_index AS dst_agent_log_index,
    dst_agent.type_index AS dst_agent_type_index,
    dst_agent.type AS dst_agent_type,
    M.dst_address,
    src_agent.name AS src_device,
    src_agent.absolute_index AS src_agent_abs_index,
    src_agent.logical_index AS src_agent_log_index,
    src_agent.type_index AS src_agent_type_index,
    src_agent.type AS src_agent_type,
    M.src_address,
    E.stack_id,
    E.parent_stack_id,
    E.correlation_id AS corr_id
FROM
    `rocpd_memory_copy` M
    INNER JOIN `rocpd_string` S ON S.id = M.name_id
    AND S.guid = M.guid
    LEFT JOIN `rocpd_string` R ON R.id = M.region_name_id
    AND R.guid = M.guid
    INNER JOIN `rocpd_info_agent` dst_agent ON dst_agent.id = M.dst_agent_id
    AND dst_agent.guid = M.guid
    INNER JOIN `rocpd_info_agent` src_agent ON src_agent.id = M.src_agent_id
    AND src_agent.guid = M.guid
    LEFT JOIN `rocpd_info_queue` Q ON Q.id = M.queue_id
    AND Q.guid = M.guid
    LEFT JOIN `rocpd_info_stream` ST ON ST.id = M.stream_id
    AND ST.guid = M.guid
    INNER JOIN `rocpd_event` E ON E.id = M.event_id
    AND E.guid = M.guid
    INNER JOIN `rocpd_info_process` P ON P.id = M.pid
    AND P.guid = M.guid
    INNER JOIN `rocpd_info_thread` T ON T.id = M.tid
    AND T.guid = M.guid;

CREATE VIEW `memory_allocations` AS
SELECT
    M.id,
    M.guid,
    (
        SELECT
            string
        FROM
            `rocpd_string` RS
        WHERE
            RS.id = E.category_id
            AND RS.guid = E.guid
    ) AS category,
    M.nid,
    P.pid,
    T.tid,
    M.start,
    M.end,
    (M.end - M.start) AS duration,
    M.type,
    M.level,
    A.name AS agent_name,
    A.absolute_index AS agent_abs_index,
    A.logical_index AS agent_log_index,
    A.type_index AS agent_type_index,
    A.type AS agent_type,
    M.address,
    M.size,
    M.queue_id,
    Q.name AS queue_name,
    M.stream_id,
    ST.name AS stream_name,
    E.stack_id,
    E.parent_stack_id,
    E.correlation_id AS corr_id
FROM
    `rocpd_memory_allocate` M
    LEFT JOIN `rocpd_info_agent` A ON M.agent_id = A.id
    AND M.guid = A.guid
    LEFT JOIN `rocpd_info_queue` Q ON Q.id = M.queue_id
    AND Q.guid = M.guid
    LEFT JOIN `rocpd_info_stream` ST ON ST.id = M.stream_id
    AND ST.guid = M.guid
    INNER JOIN `rocpd_event` E ON E.id = M.event_id
    AND E.guid = M.guid
    INNER JOIN `rocpd_info_process` P ON P.id = M.pid
    AND P.guid = M.guid
    INNER JOIN `rocpd_info_thread` T ON T.id = M.tid
    AND P.guid = M.guid;

CREATE VIEW `scratch_memory` AS
SELECT
    M.id,
    M.guid,
    M.nid,
    P.pid,
    M.type AS operation,
    A.name AS agent_name,
    A.absolute_index AS agent_abs_index,
    A.logical_index AS agent_log_index,
    A.type_index AS agent_type_index,
    A.type AS agent_type,
    M.queue_id,
    T.tid,
    JSON_EXTRACT(M.extdata, '$.flags') AS alloc_flags,
    M.start,
    M.end,
    M.size,
    M.address,
    E.correlation_id,
    E.stack_id,
    E.parent_stack_id,
    E.correlation_id AS corr_id,
    (
        SELECT
            string
        FROM
            `rocpd_string` RS
        WHERE
            RS.id = E.category_id
            AND RS.guid = E.guid
    ) AS category,
    E.extdata AS event_extdata
FROM
    `rocpd_memory_allocate` M
    LEFT JOIN `rocpd_info_agent` A ON M.agent_id = A.id
    AND M.guid = A.guid
    LEFT JOIN `rocpd_info_queue` Q ON Q.id = M.queue_id
    AND Q.guid = M.guid
    INNER JOIN `rocpd_event` E ON E.id = M.event_id
    AND E.guid = M.guid
    INNER JOIN `rocpd_info_process` P ON P.id = M.pid
    AND P.guid = M.guid
    INNER JOIN `rocpd_info_thread` T ON T.id = M.tid
    AND T.guid = M.guid
WHERE
    M.level = 'SCRATCH'
ORDER BY
    M.start ASC;

CREATE VIEW `counters_collection` AS
SELECT
    MIN(PMC_E.id) AS id,
    PMC_E.guid,
    K.dispatch_id,
    K.kernel_id,
    E.id AS event_id,
    E.correlation_id,
    E.stack_id,
    E.parent_stack_id,
    P.pid,
    T.tid,
    K.agent_id,
    A.absolute_index AS agent_abs_index,
    A.logical_index AS agent_log_index,
    A.type_index AS agent_type_index,
    A.type AS agent_type,
    K.queue_id,
    k.grid_size_x AS grid_size_x,
    k.grid_size_y AS grid_size_y,
    k.grid_size_z AS grid_size_z,
    (K.grid_size_x * K.grid_size_y * K.grid_size_z) AS grid_size,
    S.display_name AS kernel_name,
    (
        SELECT
            string
        FROM
            `rocpd_string` RS
        WHERE
            RS.id = K.region_name_id
            AND RS.guid = K.guid
    ) AS kernel_region,
    K.workgroup_size_x AS workgroup_size_x,
    K.workgroup_size_y AS workgroup_size_y,
    K.workgroup_size_z AS workgroup_size_z,
    (K.workgroup_size_x * K.workgroup_size_y * K.workgroup_size_z) AS workgroup_size,
    K.group_segment_size AS lds_block_size,
    K.private_segment_size AS scratch_size,
    S.arch_vgpr_count AS vgpr_count,
    S.accum_vgpr_count,
    S.sgpr_count,
    PMC_I.name AS counter_name,
    PMC_I.symbol AS counter_symbol,
    PMC_I.component,
    PMC_I.description,
    PMC_I.block,
    PMC_I.expression,
    PMC_I.value_type,
    PMC_I.id AS counter_id,
    SUM(PMC_E.value) AS value,
    K.start,
    K.end,
    PMC_I.is_constant,
    PMC_I.is_derived,
    (K.end - K.start) AS duration,
    (
        SELECT
            string
        FROM
            `rocpd_string` RS
        WHERE
            RS.id = E.category_id
            AND RS.guid = E.guid
    ) AS category,
    K.nid,
    E.extdata,
    S.code_object_id
FROM
    `rocpd_pmc_event` PMC_E
    INNER JOIN `rocpd_info_pmc` PMC_I ON PMC_I.id = PMC_E.pmc_id
    AND PMC_I.guid = PMC_E.guid
    INNER JOIN `rocpd_event` E ON E.id = PMC_E.event_id
    AND E.guid = PMC_E.guid
    INNER JOIN `rocpd_kernel_dispatch` K ON K.event_id = PMC_E.event_id
    AND K.guid = PMC_E.guid
    INNER JOIN `rocpd_info_agent` A ON A.id = K.agent_id
    AND A.guid = K.guid
    INNER JOIN `rocpd_info_kernel_symbol` S ON S.id = K.kernel_id
    AND S.guid = K.guid
    INNER JOIN `rocpd_info_process` P ON P.id = K.pid
    AND P.guid = K.guid
    INNER JOIN `rocpd_info_thread` T ON T.id = K.tid
    AND T.guid = K.guid
GROUP BY
    PMC_E.guid,
    K.dispatch_id,
    PMC_I.name,
    K.agent_id;

CREATE VIEW `top_kernels` AS
SELECT
    S.display_name AS name,
    COUNT(K.kernel_id) AS total_calls,
    SUM(K.end - K.start) / 1000.0 AS total_duration,
    (SUM(K.end - K.start) / COUNT(K.kernel_id)) / 1000.0 AS average,
    SUM(K.end - K.start) * 100.0 / (
        SELECT
            SUM(A.end - A.start)
        FROM
            `rocpd_kernel_dispatch` A
    ) AS percentage
FROM
    `rocpd_kernel_dispatch` K
    INNER JOIN `rocpd_info_kernel_symbol` S ON S.id = K.kernel_id
    AND S.guid = K.guid
GROUP BY
    name
ORDER BY
    total_duration DESC;

CREATE VIEW `busy` AS
SELECT
    A.agent_id,
    AG.type,
    GpuTime,
    WallTime,
    GpuTime * 1.0 / WallTime AS Busy
FROM
    (
        SELECT
            agent_id,
            guid,
            SUM(END - start) AS GpuTime
        FROM
            (
                SELECT
                    agent_id,
                    guid,
                    END,
                    start
                FROM
                    `rocpd_kernel_dispatch`
                UNION ALL
                SELECT
                    dst_agent_id AS agent_id,
                    guid,
                    END,
                    start
                FROM
                    `rocpd_memory_copy`
            )
        GROUP BY
            agent_id,
            guid
    ) A
    INNER JOIN (
        SELECT
            MAX(END) - MIN(start) AS WallTime
        FROM
            (
                SELECT
                    END,
                    start
                FROM
                    `rocpd_kernel_dispatch`
                UNION ALL
                SELECT
                    END,
                    start
                FROM
                    `rocpd_memory_copy`
            )
    ) W ON 1 = 1
    INNER JOIN `rocpd_info_agent` AG ON AG.id = A.agent_id
    AND AG.guid = A.guid;

CREATE VIEW `top` AS
SELECT
    name,
    COUNT(*) AS total_calls,
    SUM(duration) / 1000.0 AS total_duration,
    (SUM(duration) / COUNT(*)) / 1000.0 AS average,
    SUM(duration) * 100.0 / total_time AS percentage
FROM
    (
        -- Kernel operations
        SELECT
            ks.display_name AS name,
            (kd.end - kd.start) AS duration
        FROM
            `rocpd_kernel_dispatch` kd
            INNER JOIN `rocpd_info_kernel_symbol` ks ON kd.kernel_id = ks.id
            AND kd.guid = ks.guid
        UNION ALL
        -- Memory operations
        SELECT
            rs.string AS name,
            (END - start) AS duration
        FROM
            `rocpd_memory_copy` mc
            INNER JOIN `rocpd_string` rs ON rs.id = mc.name_id
            AND rs.guid = mc.guid
        UNION ALL
        -- Regions
        SELECT
            rs.string AS name,
            (END - start) AS duration
        FROM
            `rocpd_region` rr
            INNER JOIN `rocpd_string` rs ON rs.id = rr.name_id
            AND rs.guid = rr.guid
    ) operations
    CROSS JOIN (
        SELECT
            SUM(END - start) AS total_time
        FROM
            (
                SELECT
                    END,
                    start
                FROM
                    `rocpd_kernel_dispatch`
                UNION ALL
                SELECT
                    END,
                    start
                FROM
                    `rocpd_memory_copy`
                UNION ALL
                SELECT
                    END,
                    start
                FROM
                    `rocpd_region`
            )
    ) TOTAL
GROUP BY
    name
ORDER BY
    total_duration DESC;

CREATE VIEW `kernel_summary` AS
WITH
    avg_data AS (
        SELECT
            name,
            AVG(duration) AS avg_duration
        FROM
            `kernels`
        GROUP BY
            name
    ),
    aggregated_data AS (
        SELECT
            K.name,
            COUNT(*) AS calls,
            SUM(K.duration) AS total_duration,
            SUM(CAST(K.duration AS REAL) * CAST(K.duration AS REAL)) AS sqr_duration,
            A.avg_duration AS average_duration,
            MIN(K.duration) AS min_duration,
            MAX(K.duration) AS max_duration,
            SUM(CAST((K.duration - A.avg_duration) AS REAL) * CAST((K.duration - A.avg_duration) AS REAL)) / (COUNT(*) - 1) AS variance_duration,
            SQRT(
                SUM(CAST((K.duration - A.avg_duration) AS REAL) * CAST((K.duration - A.avg_duration) AS REAL)) / (COUNT(*) - 1)
            ) AS std_dev_duration
        FROM
            `kernels` K
            JOIN avg_data A ON K.name = A.name
        GROUP BY
            K.name
    ),
    total_duration AS (
        SELECT
            SUM(total_duration) AS grand_total_duration
        FROM
            aggregated_data
    )
SELECT
    AD.name AS name,
    AD.calls,
    AD.total_duration AS "DURATION (nsec)",
    AD.sqr_duration AS "SQR (nsec)",
    AD.average_duration AS "AVERAGE (nsec)",
    (CAST(AD.total_duration AS REAL) / TD.grand_total_duration) * 100 AS "PERCENT (INC)",
    AD.min_duration AS "MIN (nsec)",
    AD.max_duration AS "MAX (nsec)",
    AD.variance_duration AS "VARIANCE",
    AD.std_dev_duration AS "STD_DEV"
FROM
    aggregated_data AD
    CROSS JOIN total_duration TD;

CREATE VIEW `kernel_summary_region` AS
WITH
    avg_data AS (
        SELECT
            region,
            AVG(duration) AS avg_duration
        FROM
            `kernels`
        GROUP BY
            region
    ),
    aggregated_data AS (
        SELECT
            K.region AS name,
            COUNT(*) AS calls,
            SUM(K.duration) AS total_duration,
            SUM(CAST(K.duration AS REAL) * CAST(K.duration AS REAL)) AS sqr_duration,
            A.avg_duration AS average_duration,
            MIN(K.duration) AS min_duration,
            MAX(K.duration) AS max_duration,
            SUM(CAST((K.duration - A.avg_duration) AS REAL) * CAST((K.duration - A.avg_duration) AS REAL)) / (COUNT(*) - 1) AS variance_duration,
            SQRT(
                SUM(CAST((K.duration - A.avg_duration) AS REAL) * CAST((K.duration - A.avg_duration) AS REAL)) / (COUNT(*) - 1)
            ) AS std_dev_duration
        FROM
            `kernels` K
            JOIN avg_data A ON K.region = A.region
        GROUP BY
            K.region
    ),
    total_duration AS (
        SELECT
            SUM(total_duration) AS grand_total_duration
        FROM
            aggregated_data
    )
SELECT
    AD.name AS name,
    AD.calls,
    AD.total_duration AS "DURATION (nsec)",
    AD.sqr_duration AS "SQR (nsec)",
    AD.average_duration AS "AVERAGE (nsec)",
    (CAST(AD.total_duration AS REAL) / TD.grand_total_duration) * 100 AS "PERCENT (INC)",
    AD.min_duration AS "MIN (nsec)",
    AD.max_duration AS "MAX (nsec)",
    AD.variance_duration AS "VARIANCE",
    AD.std_dev_duration AS "STD_DEV"
FROM
    aggregated_data AD
    CROSS JOIN total_duration TD;

CREATE VIEW `memory_copy_summary` AS
WITH
    avg_data AS (
        SELECT
            name,
            AVG(duration) AS avg_duration
        FROM
            `memory_copies`
        GROUP BY
            name
    ),
    aggregated_data AS (
        SELECT
            MC.name,
            COUNT(*) AS calls,
            SUM(MC.duration) AS total_duration,
            SUM(CAST(MC.duration AS REAL) * CAST(MC.duration AS REAL)) AS sqr_duration,
            A.avg_duration AS average_duration,
            MIN(MC.duration) AS min_duration,
            MAX(MC.duration) AS max_duration,
            SUM(
                CAST((MC.duration - A.avg_duration) AS REAL) * CAST((MC.duration - A.avg_duration) AS REAL)
            ) / (COUNT(*) - 1) AS variance_duration,
            SQRT(
                SUM(
                    CAST((MC.duration - A.avg_duration) AS REAL) * CAST((MC.duration - A.avg_duration) AS REAL)
                ) / (COUNT(*) - 1)
            ) AS std_dev_duration
        FROM
            `memory_copies` MC
            JOIN avg_data A ON MC.name = A.name
        GROUP BY
            MC.name
    ),
    total_duration AS (
        SELECT
            SUM(total_duration) AS grand_total_duration
        FROM
            aggregated_data
    )
SELECT
    AD.name AS name,
    AD.calls,
    AD.total_duration AS "DURATION (nsec)",
    AD.sqr_duration AS "SQR (nsec)",
    AD.average_duration AS "AVERAGE (nsec)",
    (CAST(AD.total_duration AS REAL) / TD.grand_total_duration) * 100 AS "PERCENT (INC)",
    AD.min_duration AS "MIN (nsec)",
    AD.max_duration AS "MAX (nsec)",
    AD.variance_duration AS "VARIANCE",
    AD.std_dev_duration AS "STD_DEV"
FROM
    aggregated_data AD
    CROSS JOIN total_duration TD;

CREATE VIEW `memory_allocation_summary` AS
WITH
    avg_data AS (
        SELECT
            type AS name,
            AVG(duration) AS avg_duration
        FROM
            `memory_allocations`
        GROUP BY
            type
    ),
    aggregated_data AS (
        SELECT
            MA.type AS name,
            COUNT(*) AS calls,
            SUM(MA.duration) AS total_duration,
            SUM(CAST(MA.duration AS REAL) * CAST(MA.duration AS REAL)) AS sqr_duration,
            A.avg_duration AS average_duration,
            MIN(MA.duration) AS min_duration,
            MAX(MA.duration) AS max_duration,
            SUM(
                CAST((MA.duration - A.avg_duration) AS REAL) * CAST((MA.duration - A.avg_duration) AS REAL)
            ) / (COUNT(*) - 1) AS variance_duration,
            SQRT(
                SUM(
                    CAST((MA.duration - A.avg_duration) AS REAL) * CAST((MA.duration - A.avg_duration) AS REAL)
                ) / (COUNT(*) - 1)
            ) AS std_dev_duration
        FROM
            `memory_allocations` MA
            JOIN avg_data A ON MA.type = A.name
        GROUP BY
            MA.type
    ),
    total_duration AS (
        SELECT
            SUM(total_duration) AS grand_total_duration
        FROM
            aggregated_data
    )
SELECT
    'MEMORY_ALLOCATION_' || AD.name AS name,
    AD.calls,
    AD.total_duration AS "DURATION (nsec)",
    AD.sqr_duration AS "SQR (nsec)",
    AD.average_duration AS "AVERAGE (nsec)",
    (CAST(AD.total_duration AS REAL) / TD.grand_total_duration) * 100 AS "PERCENT (INC)",
    AD.min_duration AS "MIN (nsec)",
    AD.max_duration AS "MAX (nsec)",
    AD.variance_duration AS "VARIANCE",
    AD.std_dev_duration AS "STD_DEV"
FROM
    aggregated_data AD
    CROSS JOIN total_duration TD;
