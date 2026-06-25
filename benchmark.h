#pragma once
#include "memory.h"
#include <vector>
#include <string>
#include <chrono>
#include <random>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <fstream>

// ---------------------------------------------------------------
// Result of a single benchmark run
// ---------------------------------------------------------------

struct BenchmarkResult {
    std::string impl_name;
    std::string workload_name;
    double avg_latency_ns       = 0.0;
    double min_latency_ns       = 0.0;
    double max_latency_ns       = 0.0;
    double stddev_latency_ns    = 0.0;
    double throughput_ops_sec   = 0.0;
    size_t memory_overhead_bytes = 0;
    size_t live_allocations      = 0;
    int    correctness_violations = 0;
    AccessStats stats;
};

// ---------------------------------------------------------------
// Timing helper — wraps clock_gettime via chrono steady_clock
// ---------------------------------------------------------------

class Timer {
public:
    void start() { t0_ = std::chrono::steady_clock::now(); }

    double stop_ns() {
        auto t1 = std::chrono::steady_clock::now();
        return std::chrono::duration<double, std::nano>(t1 - t0_).count();
    }

private:
    std::chrono::steady_clock::time_point t0_;
};

// ---------------------------------------------------------------
// Statistics over a vector of latency samples
// ---------------------------------------------------------------

inline void compute_stats(std::vector<double>& samples,
                          double& avg, double& mn, double& mx, double& stddev) {
    std::sort(samples.begin(), samples.end());
    double sum = std::accumulate(samples.begin(), samples.end(), 0.0);
    avg = sum / samples.size();
    mn  = samples.front();
    mx  = samples.back();

    double sq_sum = 0.0;
    for (double s : samples) sq_sum += (s - avg) * (s - avg);
    stddev = std::sqrt(sq_sum / samples.size());
}

// ---------------------------------------------------------------
// CSV writer — appends results for later plotting
// ---------------------------------------------------------------

class ResultWriter {
public:
    explicit ResultWriter(const std::string& path) : out_(path) {
        out_ << "impl,workload,avg_latency_ns,min_latency_ns,max_latency_ns,"
                "stddev_latency_ns,throughput_ops_sec,memory_overhead_bytes,"
                "live_allocations,correctness_violations,tag_accesses,"
                "pointer_accesses,bytes_transferred,alloc_count,free_count,"
                "bytes_allocated,simulated_accesses,peak_overhead_bytes\n";
    }

    void write(const BenchmarkResult& r) {
        out_ << r.impl_name << ","
             << r.workload_name << ","
             << r.avg_latency_ns << ","
             << r.min_latency_ns << ","
             << r.max_latency_ns << ","
             << r.stddev_latency_ns << ","
             << r.throughput_ops_sec << ","
             << r.memory_overhead_bytes << ","
             << r.live_allocations << ","
             << r.correctness_violations << ","
             << r.stats.tag_accesses << ","
             << r.stats.pointer_accesses << ","
             << r.stats.bytes_transferred << ","
             << r.stats.alloc_count << ","
             << r.stats.free_count << ","
             << r.stats.bytes_allocated << ","
             << r.stats.simulated_accesses << ","
             << r.stats.peak_overhead_bytes << "\n";
    }

private:
    std::ofstream out_;
};

inline void print_result(const BenchmarkResult& r) {
    std::cout << "[" << r.impl_name << "] " << r.workload_name << "\n"
              << "  avg latency   : " << r.avg_latency_ns << " ns\n"
              << "  min/max       : " << r.min_latency_ns << " / "
                                       << r.max_latency_ns << " ns\n"
              << "  stddev        : " << r.stddev_latency_ns << " ns\n"
              << "  throughput    : " << r.throughput_ops_sec << " ops/sec\n"
              << "  mem overhead  : " << r.memory_overhead_bytes << " bytes\n"
              << "  live allocs   : " << r.live_allocations << "\n"
              << "  violations    : " << r.correctness_violations << "\n"
              << "  stats         : tag_accesses=" << r.stats.tag_accesses
              << ", pointer_accesses=" << r.stats.pointer_accesses
              << ", bytes_transferred=" << r.stats.bytes_transferred
              << ", alloc_count=" << r.stats.alloc_count
              << ", free_count=" << r.stats.free_count
              << ", bytes_allocated=" << r.stats.bytes_allocated
              << ", simulated_accesses=" << r.stats.simulated_accesses
              << ", peak_overhead_bytes=" << r.stats.peak_overhead_bytes
              << "\n\n";
}
