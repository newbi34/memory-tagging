#pragma once
#include <vector>
#include <string>
#include <random>
#include <cstdint>

// ---------------------------------------------------------------
// A workload is a fixed sequence of alloc sizes and an access
// pattern. It does NOT contain raw addresses, since addresses are
// only known once Memory::alloc() actually runs for each impl.
// ---------------------------------------------------------------

enum class AccessType { SEQUENTIAL, RANDOM };

struct WorkloadSpec {
    std::string name;
    std::vector<size_t> alloc_sizes;  // size of each allocation, in order
    int    accesses_per_alloc = 8;    // how many check_tag calls per alloc
    AccessType access_pattern = AccessType::SEQUENTIAL;
    bool   include_free_cycle = false; // free half, realloc, used for churn test
};

class Workload {
public:
    static WorkloadSpec small_allocs(size_t count = 2000) {
        WorkloadSpec w;
        w.name = "small_allocs";
        w.alloc_sizes.assign(count, 32); // 32B each
        w.accesses_per_alloc = 4;
        w.access_pattern = AccessType::SEQUENTIAL;
        return w;
    }

    static WorkloadSpec large_allocs(size_t count = 50) {
        WorkloadSpec w;
        w.name = "large_allocs";
        w.alloc_sizes.assign(count, 64 * 1024); // 64KB each
        w.accesses_per_alloc = 16;
        w.access_pattern = AccessType::SEQUENTIAL;
        return w;
    }

    static WorkloadSpec mixed_sizes(size_t count = 1000) {
        WorkloadSpec w;
        w.name = "mixed_sizes";
        std::mt19937 rng(42); // fixed seed — reproducible across impls
        std::uniform_int_distribution<size_t> dist(16, 2048);
        for (size_t i = 0; i < count; ++i)
            w.alloc_sizes.push_back(dist(rng));
        w.accesses_per_alloc = 8;
        w.access_pattern = AccessType::RANDOM;
        return w;
    }

    static WorkloadSpec high_churn(size_t count = 1000) {
        WorkloadSpec w;
        w.name = "high_churn";
        w.alloc_sizes.assign(count, 48);
        w.accesses_per_alloc = 4;
        w.access_pattern = AccessType::SEQUENTIAL;
        w.include_free_cycle = true;
        return w;
    }

    static WorkloadSpec random_access(size_t count = 1000) {
        WorkloadSpec w;
        w.name = "random_access";
        w.alloc_sizes.assign(count, 64);
        w.accesses_per_alloc = 8;
        w.access_pattern = AccessType::RANDOM;
        return w;
    }

    static std::vector<WorkloadSpec> all() {
        return {
            small_allocs(),
            large_allocs(),
            mixed_sizes(),
            high_churn(),
            random_access()
        };
    }
};
