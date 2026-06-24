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
    ResultWriter writer("benchmark_results.csv");
    auto workloads = Workload::all();

    for (const auto& w : workloads) {
        std::cout << "=== Workload: " << w.name << " ===\n\n";

        {
            BasicShadow mem(BENCH_MEM_SIZE, 1);
            BenchmarkResult r = BenchmarkRunner::run(mem, w);
            print_result(r);
            writer.write(r);
        }
        {
            BasicShadow mem(BENCH_MEM_SIZE, 2);
            BenchmarkResult r = BenchmarkRunner::run(mem, w);
            print_result(r);
            writer.write(r);
        }
        {
            BasicShadow mem(BENCH_MEM_SIZE, 4);
            BenchmarkResult r = BenchmarkRunner::run(mem, w);
            print_result(r);
            writer.write(r);
        }
        {
            BasicShadow mem(BENCH_MEM_SIZE, 8);
            BenchmarkResult r = BenchmarkRunner::run(mem, w);
            print_result(r);
            writer.write(r);
        }
        {
            BasicShadow mem(BENCH_MEM_SIZE, 16);
            BenchmarkResult r = BenchmarkRunner::run(mem, w);
            print_result(r);
            writer.write(r);
        }
        {
            BasicShadow mem(BENCH_MEM_SIZE, 32);
            BenchmarkResult r = BenchmarkRunner::run(mem, w);
            print_result(r);
            writer.write(r);
        }
        {
            BasicShadow mem(BENCH_MEM_SIZE, 64);
            BenchmarkResult r = BenchmarkRunner::run(mem, w);
            print_result(r);
            writer.write(r);
        }
        {
            BasicShadow mem(BENCH_MEM_SIZE, 128);
            BenchmarkResult r = BenchmarkRunner::run(mem, w);
            print_result(r);
            writer.write(r);
        }
        {
            TwoLevelTable mem(BENCH_MEM_SIZE);
            BenchmarkResult r = BenchmarkRunner::run(mem, w);
            print_result(r);
            writer.write(r);
        }
        {
            BalancedTree mem(BENCH_MEM_SIZE);
            BenchmarkResult r = BenchmarkRunner::run(mem, w);
            print_result(r);
            writer.write(r);
        }
        {
            CuckooHashTable mem(BENCH_MEM_SIZE);
            BenchmarkResult r = BenchmarkRunner::run(mem, w);
            print_result(r);
            writer.write(r);
        }
    }

    std::cout << "Results written to benchmark_results.csv\n";
    return 0;
}
