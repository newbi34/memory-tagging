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

    // Runs `w` against `mem` and returns timing + memory results.
    // `mem` should be freshly constructed (empty) before calling.
    static BenchmarkResult run(Memory& mem, const WorkloadSpec& w) {
        BenchmarkResult result;
        result.impl_name     = mem.name();
        result.workload_name = w.name;

        std::vector<double> latencies;
        latencies.reserve(NUM_RUNS);

        for (int run = 0; run < NUM_RUNS; ++run) {
            double ns = execute_once(mem, w, result);
            latencies.push_back(ns);
            //mem.reset_shadow();
        }

        double avg, mn, mx, stddev;
        compute_stats(latencies, avg, mn, mx, stddev);

        result.avg_latency_ns    = avg;
        result.min_latency_ns    = mn;
        result.max_latency_ns    = mx;
        result.stddev_latency_ns = stddev;
        result.stats = mem.get_stats();

        // throughput based on total ops per run (allocs + accesses + frees)
        size_t ops_per_run = w.alloc_sizes.size() *
                            (1 + w.accesses_per_alloc +
                             (w.include_free_cycle ? 1 : 0));
        result.throughput_ops_sec = ops_per_run / (avg / 1e9);

        result.memory_overhead_bytes = mem.get_stats().peak_overhead_bytes;
        //result.memory_overhead_bytes = mem.get_overhead_bytes();

        return result;
    }

private:
    // Executes the full workload once against mem, returns elapsed ns.
    // mem state is reset (cleared) before and after via fresh alloc/free
    // pairs, so the implementation always starts from a known state.
    static double execute_once(Memory& mem, const WorkloadSpec& w, BenchmarkResult& result) {
        struct LiveAlloc { uint64_t addr; uint8_t tag; };
        std::vector<LiveAlloc> live;

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
            live.push_back({addr, tag});

            if (w.include_global_random_access) goto skip;

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

            skip:
            // random global access test: check a random address in the entire memory space
            if (w.include_global_random_access) {
                size_t mem_size = mem.get_size();
                std::uniform_int_distribution<uint64_t> global_d(0, mem_size - 1);
                uint64_t rand_addr = global_d(rng);

                // find which allocation this address belongs to, if any
                uint8_t expected_tag = 0; // assume untagged
                for (size_t j = 0; j < live.size(); ++j) {
                    size_t alloc_sz = mem.alloc_size(live[j].addr);
                    if (rand_addr >= live[j].addr && rand_addr < live[j].addr + alloc_sz) {
                        expected_tag = live[j].tag;
                        break;
                    }
                }
            
                // now check_tag with the correct expected tag
                bool ok = mem.check_tag(rand_addr, expected_tag);
                if (!ok) result.correctness_violations++;
            }
        }

        double elapsed = timer.stop_ns();

        result.live_allocations = live.size();

        // clean up remaining live allocations so mem can be reused
        // for the next run without growing unbounded
        for (const auto& la : live) mem.free(la.addr);

        return elapsed;
    }
};
