#pragma once

// Subcommands for split read benchmarking (prepare DBs once; one process per read).
int runPrepareReadDbs(int argc, char** argv);
int runReadOnceCli(int argc, char** argv);
