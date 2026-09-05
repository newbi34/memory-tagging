#include "memory.h"
#include "two_level_table.cpp"
#include "balanced_tree.cpp"
#include "hash_table.cpp"
#include "basic_shadow.cpp"
#include "benchmark.h"
#include "workload.h"
#include "benchmark_runner.h"
#include <iostream>
#include <memory>

// ---------------------------------------------------------------
// Memory size used for all benchmarks. Keep consistent across
// implementations and workloads so results are comparable.
// ---------------------------------------------------------------
constexpr size_t BENCH_MEM_SIZE = 64 * 1024 * 1024; // 64MB

int main() {
    ResultWriter writer("test10.csv");
    auto workloads = Workload::all();

    for (const auto& w : workloads) {
        std::cout << "=== Workload: " << w.name << " ===\n\n";

        /*{ BasicShadow mem(BENCH_MEM_SIZE, 15); BenchmarkResult r = BenchmarkRunner::run(mem, w); print_result(r); writer.write(r); }
        { BasicShadow mem(BENCH_MEM_SIZE, 16); BenchmarkResult r = BenchmarkRunner::run(mem, w); print_result(r); writer.write(r); }
        { BasicShadow mem(BENCH_MEM_SIZE, 17); BenchmarkResult r = BenchmarkRunner::run(mem, w); print_result(r); writer.write(r); }*/
        /*{ BasicShadow mem(BENCH_MEM_SIZE, 4); BenchmarkResult r = BenchmarkRunner::run(mem, w); print_result(r); writer.write(r); }
        { BasicShadow mem(BENCH_MEM_SIZE, 8); BenchmarkResult r = BenchmarkRunner::run(mem, w); print_result(r); writer.write(r); }
        { BasicShadow mem(BENCH_MEM_SIZE, 16); BenchmarkResult r = BenchmarkRunner::run(mem, w); print_result(r); writer.write(r); }
        { BasicShadow mem(BENCH_MEM_SIZE, 32); BenchmarkResult r = BenchmarkRunner::run(mem, w); print_result(r); writer.write(r); }
        { BasicShadow mem(BENCH_MEM_SIZE, 64); BenchmarkResult r = BenchmarkRunner::run(mem, w); print_result(r); writer.write(r); }
        { BasicShadow mem(BENCH_MEM_SIZE, 128); BenchmarkResult r = BenchmarkRunner::run(mem, w); print_result(r); writer.write(r); }*/

        /*{ TwoLevelTable mem(BENCH_MEM_SIZE, 3900, 30); BenchmarkResult r = BenchmarkRunner::run(mem, w); print_result(r); writer.write(r); mem.print_overhead_bytes();}
        { TwoLevelTable mem(BENCH_MEM_SIZE, 4096, 32); BenchmarkResult r = BenchmarkRunner::run(mem, w); print_result(r); writer.write(r); mem.print_overhead_bytes();}
        { TwoLevelTable mem(BENCH_MEM_SIZE, 4200, 35); BenchmarkResult r = BenchmarkRunner::run(mem, w); print_result(r); writer.write(r); mem.print_overhead_bytes();}*/
        { TwoLevelTable mem(BENCH_MEM_SIZE, 2048, 8); BenchmarkResult r = BenchmarkRunner::run(mem, w); print_result(r); writer.write(r); mem.print_overhead_bytes();}
        { TwoLevelTable mem(BENCH_MEM_SIZE, 2048, 16); BenchmarkResult r = BenchmarkRunner::run(mem, w); print_result(r); writer.write(r); mem.print_overhead_bytes();}
        { TwoLevelTable mem(BENCH_MEM_SIZE, 2048, 32); BenchmarkResult r = BenchmarkRunner::run(mem, w); print_result(r); writer.write(r); mem.print_overhead_bytes();}
        /*{ TwoLevelTable mem(BENCH_MEM_SIZE, 4096, 16); BenchmarkResult r = BenchmarkRunner::run(mem, w); print_result(r); writer.write(r); mem.print_overhead_bytes();}
        { TwoLevelTable mem(BENCH_MEM_SIZE, 4096, 32); BenchmarkResult r = BenchmarkRunner::run(mem, w); print_result(r); writer.write(r); mem.print_overhead_bytes();}
        { TwoLevelTable mem(BENCH_MEM_SIZE, 8192, 32); BenchmarkResult r = BenchmarkRunner::run(mem, w); print_result(r); writer.write(r); mem.print_overhead_bytes();}
        { TwoLevelTable mem(BENCH_MEM_SIZE, 8192, 64); BenchmarkResult r = BenchmarkRunner::run(mem, w); print_result(r); writer.write(r); mem.print_overhead_bytes();}
        { TwoLevelTable mem(BENCH_MEM_SIZE, 16384, 64); BenchmarkResult r = BenchmarkRunner::run(mem, w); print_result(r); writer.write(r); mem.print_overhead_bytes();}*/

        { BalancedTree mem(BENCH_MEM_SIZE); BenchmarkResult r = BenchmarkRunner::run(mem, w); print_result(r); writer.write(r); }

        /*{ CuckooHashTable mem(BENCH_MEM_SIZE, 512, 8); BenchmarkResult r = BenchmarkRunner::run(mem, w); print_result(r); writer.write(r);}
        { CuckooHashTable mem(BENCH_MEM_SIZE, 1024, 16); BenchmarkResult r = BenchmarkRunner::run(mem, w); print_result(r); writer.write(r);}
        { CuckooHashTable mem(BENCH_MEM_SIZE, 1024, 32); BenchmarkResult r = BenchmarkRunner::run(mem, w); print_result(r); writer.write(r);}
        { CuckooHashTable mem(BENCH_MEM_SIZE, 2048, 32); BenchmarkResult r = BenchmarkRunner::run(mem, w); print_result(r); writer.write(r);}
        { CuckooHashTable mem(BENCH_MEM_SIZE, 2048, 64); BenchmarkResult r = BenchmarkRunner::run(mem, w); print_result(r); writer.write(r);}
        { CuckooHashTable mem(BENCH_MEM_SIZE, 4096, 128); BenchmarkResult r = BenchmarkRunner::run(mem, w); print_result(r); writer.write(r);}*/
    }

    std::cout << "Results written to benchmark_results.csv\n";
    return 0;
}
