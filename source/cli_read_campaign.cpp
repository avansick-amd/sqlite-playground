#include "cli_read_campaign.h"
#include "asset_paths.h"
#include "benchmarks.h"
#include "db_helpers.h"
#include "schema_combined.h"
#include "schema_original.h"
#include <benchmark/benchmark.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

namespace {

constexpr const char* kReadBenchmarkNames[] = {
    "BM_Read_Original",
    "BM_Read_Original_Indexed_Initial",
    "BM_Read_Original_Indexed_Deferred",
    "BM_Read_Combined",
    "BM_Read_Combined_Indexed_Initial",
    "BM_Read_Combined_Indexed_Deferred",
};

bool isKnownReadCase(const std::string& name) {
    for (const char* n : kReadBenchmarkNames) {
        if (name == n) {
            return true;
        }
    }
    return false;
}

void usagePrepare() {
    std::cerr
        << "Usage: benchmark_schema prepare-read-dbs [--db-dir DIR]\n"
        << "  Creates the six read benchmark databases (same layout as BM_Read_*).\n"
        << "  Default DIR is the current working directory.\n";
}

void usageRead() {
    std::cerr
        << "Usage: benchmark_schema read --case NAME [--db-dir DIR] [--jsonl FILE] "
           "[--iteration N]\n"
        << "  Runs one full read in a fresh process; does not load the source ROCpd DB.\n"
        << "  Appends one JSON object per line to FILE (or prints one line to stdout if "
           "omitted).\n";
}

bool parseDbDir(int argc, char** argv, int start, std::string& out_dir) {
    out_dir = ".";
    for (int i = start; i < argc; ++i) {
        if (std::strcmp(argv[i], "--db-dir") == 0) {
            if (i + 1 >= argc) {
                std::cerr << "error: --db-dir needs a path\n";
                return false;
            }
            out_dir = argv[++i];
        } else {
            std::cerr << "error: unknown argument: " << argv[i] << "\n";
            return false;
        }
    }
    return true;
}

void materializeReadDatabase(const std::string& name, const std::string& db_path,
                             const std::vector<KernelDispatch>& dispatches,
                             const RefData& refs) {
    if (name == "BM_Read_Original") {
        sqlite3* db_load =
            createDatabase(benchmarkAsset("rocpd_tables.sql"), db_path, true, false);
        populateOriginalRefs(db_load, refs);
        insertOriginalDispatches(db_load, dispatches, false);
        sqlite3_close(db_load);
        return;
    }
    if (name == "BM_Read_Original_Indexed_Initial") {
        sqlite3* db_load =
            createDatabase(benchmarkAsset("rocpd_tables.sql"), db_path, true, false);
        executeSqlFile(db_load, benchmarkAsset("rocpd_indexes.sql"));
        populateOriginalRefs(db_load, refs);
        insertOriginalDispatches(db_load, dispatches, false);
        sqlite3_close(db_load);
        return;
    }
    if (name == "BM_Read_Original_Indexed_Deferred") {
        sqlite3* db_load =
            createDatabase(benchmarkAsset("rocpd_tables.sql"), db_path, true, false);
        populateOriginalRefs(db_load, refs);
        insertOriginalDispatches(db_load, dispatches, false);
        executeSqlFile(db_load, benchmarkAsset("rocpd_indexes.sql"));
        sqlite3_close(db_load);
        return;
    }
    if (name == "BM_Read_Combined") {
        sqlite3* db_load = createDatabase(benchmarkAsset("rocpd_tables_combined.sql"),
                                           db_path, true, false);
        populateCombinedRefs(db_load, refs, dispatches);
        insertCombinedDispatches(db_load, dispatches, false);
        sqlite3_close(db_load);
        return;
    }
    if (name == "BM_Read_Combined_Indexed_Initial") {
        sqlite3* db_load = createDatabase(benchmarkAsset("rocpd_tables_combined.sql"),
                                          db_path, true, false);
        executeSqlFile(db_load, benchmarkAsset("rocpd_combined_indexes.sql"));
        populateCombinedRefs(db_load, refs, dispatches);
        insertCombinedDispatches(db_load, dispatches, false);
        sqlite3_close(db_load);
        return;
    }
    if (name == "BM_Read_Combined_Indexed_Deferred") {
        sqlite3* db_load = createDatabase(benchmarkAsset("rocpd_tables_combined.sql"),
                                           db_path, true, false);
        populateCombinedRefs(db_load, refs, dispatches);
        insertCombinedDispatches(db_load, dispatches, false);
        executeSqlFile(db_load, benchmarkAsset("rocpd_combined_indexes.sql"));
        sqlite3_close(db_load);
        return;
    }
    std::cerr << "internal error: unhandled read case " << name << "\n";
    std::exit(1);
}

bool isCombinedReadName(const std::string& name) {
    return name.find("Read_Combined") != std::string::npos;
}

}  // namespace

int runPrepareReadDbs(int argc, char** argv) {
    std::string db_dir_arg;
    if (!parseDbDir(argc, argv, 2, db_dir_arg)) {
        usagePrepare();
        return 2;
    }

    namespace fs = std::filesystem;
    const fs::path db_dir_path =
        (db_dir_arg.empty() || db_dir_arg == ".")
            ? fs::current_path()
            : fs::absolute(db_dir_arg);
    std::error_code mkdir_ec;
    fs::create_directories(db_dir_path, mkdir_ec);
    if (mkdir_ec) {
        std::cerr << "error: cannot create db directory " << db_dir_path.string()
                  << ": " << mkdir_ec.message() << "\n";
        return 1;
    }

    ensureBenchmarkDataLoaded();
    for (const char* name : kReadBenchmarkNames) {
        const fs::path db_file = db_dir_path / (std::string(name) + ".db");
        const std::string path = db_file.string();
        std::fprintf(stderr, "materializing %s -> %s\n", name, path.c_str());
        materializeReadDatabase(name, path, g_dispatches, g_refs);
    }
    std::fprintf(stderr, "done: %zu databases\n",
                 sizeof(kReadBenchmarkNames) / sizeof(kReadBenchmarkNames[0]));
    return 0;
}

int runReadOnceCli(int argc, char** argv) {
    std::string case_name;
    std::string db_dir = ".";
    std::string jsonl_path;
    long long iteration = 0;

    for (int i = 2; i < argc; ++i) {
        if (std::strcmp(argv[i], "--case") == 0) {
            if (i + 1 >= argc) {
                std::cerr << "error: --case needs a value\n";
                usageRead();
                return 2;
            }
            case_name = argv[++i];
        } else if (std::strcmp(argv[i], "--db-dir") == 0) {
            if (i + 1 >= argc) {
                std::cerr << "error: --db-dir needs a path\n";
                usageRead();
                return 2;
            }
            db_dir = argv[++i];
        } else if (std::strcmp(argv[i], "--jsonl") == 0) {
            if (i + 1 >= argc) {
                std::cerr << "error: --jsonl needs a path\n";
                usageRead();
                return 2;
            }
            jsonl_path = argv[++i];
        } else if (std::strcmp(argv[i], "--iteration") == 0) {
            if (i + 1 >= argc) {
                std::cerr << "error: --iteration needs a number\n";
                usageRead();
                return 2;
            }
            char* end = nullptr;
            iteration = std::strtoll(argv[++i], &end, 10);
            if (end == argv[i] || *end != '\0') {
                std::cerr << "error: invalid --iteration\n";
                return 2;
            }
        } else {
            std::cerr << "error: unknown argument: " << argv[i] << "\n";
            usageRead();
            return 2;
        }
    }

    if (case_name.empty()) {
        std::cerr << "error: --case is required\n";
        usageRead();
        return 2;
    }
    if (!isKnownReadCase(case_name)) {
        std::cerr << "error: unknown read case \"" << case_name << "\"\n";
        return 2;
    }

    namespace fs = std::filesystem;
    const fs::path db_dir_path =
        (db_dir.empty() || db_dir == ".") ? fs::current_path() : fs::absolute(db_dir);
    const std::string db_path = (db_dir_path / (case_name + ".db")).string();
    std::ifstream probe(db_path);
    if (!probe) {
        std::cerr << "error: database not found: " << db_path << "\n";
        std::cerr << "run: benchmark_schema prepare-read-dbs --db-dir <dir>\n";
        return 1;
    }
    probe.close();

    sqlite3* db_read = nullptr;
    if (sqlite3_open(db_path.c_str(), &db_read) != SQLITE_OK) {
        std::cerr << "error: sqlite3_open failed\n";
        return 1;
    }
    applyReadBenchmarkPragmas(db_read);

    using clock = std::chrono::steady_clock;
    const auto t0 = clock::now();
    int64_t n = 0;
    if (isCombinedReadName(case_name)) {
        n = readCombinedDispatches(db_read);
    } else {
        n = readOriginalDispatches(db_read);
    }
    const auto t1 = clock::now();
    benchmark::DoNotOptimize(n);

    sqlite3_close(db_read);

    const double sec =
        std::chrono::duration<double>(t1 - t0).count();

    std::ostringstream line;
    line << std::setprecision(15) << std::fixed;
    line << "{\"benchmark\":\"" << case_name << "\",\"iteration\":" << iteration
         << ",\"real_time_sec\":" << sec << ",\"items\":" << n << "}\n";

    const std::string s = line.str();
    if (jsonl_path.empty()) {
        std::cout << s;
    } else {
        std::ofstream out(jsonl_path, std::ios::app);
        if (!out) {
            std::cerr << "error: cannot open jsonl: " << jsonl_path << "\n";
            return 1;
        }
        out << s;
    }

    return 0;
}
