#pragma once
#include "memory.h"
#include "workload.h"
#include "benchmark.h"
#include <vector>
#include <random>

// ---------------------------------------------------------------
// BenchmarkRunner — executes a WorkloadSpec against any Memory
// subclass and produces a BenchmarkResult
// ---------------------------------------------------------------

class BenchmarkRunner {
public:
    static constexpr int NUM_RUNS = 20;
    static constexpr int WARMUP_RUNS = 1;

    // Runs `w` against `mem` and returns timing + memory results.
    // `mem` should be freshly constructed (empty) before calling.
    static BenchmarkResult run(Memory& mem, const WorkloadSpec& w) {
        BenchmarkResult result;
        result.impl_name     = mem.name();
        result.workload_name = w.name;

        std::vector<double> latencies;
        latencies.reserve(NUM_RUNS);

        for (int run = 0; run < WARMUP_RUNS + NUM_RUNS; ++run) {
            double ns = execute_once(mem, w, result);
            if (run >= WARMUP_RUNS) // discard warmup sample
                latencies.push_back(ns);
        }

        double avg, mn, mx, stddev;
        compute_stats(latencies, avg, mn, mx, stddev);

        result.avg_latency_ns    = avg;
        result.min_latency_ns    = mn;
        result.max_latency_ns    = mx;
        result.stddev_latency_ns = stddev;

        // throughput based on total ops per run (allocs + accesses + frees)
        size_t ops_per_run = w.alloc_sizes.size() *
                            (1 + w.accesses_per_alloc +
                             (w.include_free_cycle ? 1 : 0));
        result.throughput_ops_sec = ops_per_run / (avg / 1e9);

        result.memory_overhead_bytes = mem.get_overhead_bytes();

        return result;
    }

private:
    // Executes the full workload once against mem, returns elapsed ns.
    // mem state is reset (cleared) before and after via fresh alloc/free
    // pairs, so the implementation always starts from a known state.
    static double execute_once(Memory& mem, const WorkloadSpec& w,
                               BenchmarkResult& result) {
        std::vector<uint64_t> live;
        live.reserve(w.alloc_sizes.size());

        std::mt19937 rng(1234); // fixed seed for reproducible access pattern

        Timer timer;
        timer.start();

        for (size_t i = 0; i < w.alloc_sizes.size(); ++i) {
            size_t  sz  = w.alloc_sizes[i];
            uint8_t tag = static_cast<uint8_t>((i % 15) + 1); // avoid tag 0

            uint64_t addr = mem.alloc(sz, tag);
            if (addr == Memory::INVALID) {
                // allocator ran out of space — stop this run early
                break;
            }
            live.push_back(addr);

            // perform accesses_per_alloc check_tag calls on this allocation
            for (int a = 0; a < w.accesses_per_alloc; ++a) {
                uint64_t offset;
                if (w.access_pattern == AccessType::SEQUENTIAL) {
                    offset = (a * 16) % std::max<size_t>(sz, 16);
                } else {
                    std::uniform_int_distribution<uint64_t> d(
                        0, std::max<size_t>(sz, 16) - 1);
                    offset = d(rng);
                }

                bool ok = mem.check_tag(addr + offset, tag);
                if (!ok) result.correctness_violations++;
            }

            // optional churn: free every other allocation immediately
            if (w.include_free_cycle && (i % 2 == 0)) {
                mem.free(addr);
                live.pop_back();
            }
        }

        double elapsed = timer.stop_ns();

        result.live_allocations = live.size();

        // clean up remaining live allocations so mem can be reused
        // for the next run without growing unbounded
        for (uint64_t a : live) mem.free(a);

        return elapsed;
    }
};
