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
    int    accesses_per_alloc = 16;    // how many check_tag calls per alloc
    AccessType access_pattern = AccessType::SEQUENTIAL;
    bool   include_free_cycle = false; // free half, realloc, used for churn test
    bool  include_global_random_access = false; // perform random accesses across all allocations, used for global random access test
};

//320 KB 0.5% redko

//16 MB 25% gosto

//redko

class Workload {
public:
    static WorkloadSpec small_sizes(size_t count = 10000) {
        WorkloadSpec w;
        w.name = "small_sizes";
        w.alloc_sizes.assign(count, 32); // 32B each
        w.accesses_per_alloc = 16;
        w.access_pattern = AccessType::SEQUENTIAL;
        return w;
    }

    static WorkloadSpec large_sizes(size_t count = 50) {
        WorkloadSpec w;
        w.name = "large_sizes";
        w.alloc_sizes.assign(count, 64 * 100); // 6.4KB each
        w.accesses_per_alloc = 16;
        w.access_pattern = AccessType::SEQUENTIAL;
        return w;
    }

    static WorkloadSpec mixed_sizes(size_t count = 1000) {
        WorkloadSpec w;
        w.name = "mixed_sizes";
        std::mt19937 rng(42); // fixed seed — reproducible across impls
        std::uniform_int_distribution<size_t> dist(16, 624); // average 3.2KB
        for (size_t i = 0; i < count; ++i)
            w.alloc_sizes.push_back(dist(rng));
        w.accesses_per_alloc = 16;
        w.access_pattern = AccessType::RANDOM;
        return w;
    }

    static WorkloadSpec realloc(size_t count = 1000) {
        WorkloadSpec w;
        w.name = "realloc";
        w.alloc_sizes.assign(count, 320); // 3200B each
        w.accesses_per_alloc = 16;
        w.access_pattern = AccessType::SEQUENTIAL;
        w.include_free_cycle = true;
        return w;
    }

    static WorkloadSpec random_access(size_t count = 1000) {
        WorkloadSpec w;
        w.name = "random_access";
        w.alloc_sizes.assign(count, 320);
        w.accesses_per_alloc = 16;
        w.access_pattern = AccessType::RANDOM;
        return w;
    }

    //4. few allocations, random accesses. (done)
    static WorkloadSpec global_random_access(size_t count = 1000) {
        WorkloadSpec w;
        w.name = "global_random_access";
        w.alloc_sizes.assign(count, 320);
        w.accesses_per_alloc = 16;
        w.access_pattern = AccessType::RANDOM;
        w.include_global_random_access = true;
        return w;
    }

    static std::vector<WorkloadSpec> all() {
        return {
            small_sizes(),
            large_sizes(),
            mixed_sizes(),
            realloc(),
            random_access(),
            global_random_access()
        };
    }
};

//gosto

/*
class Workload {
public:
    static WorkloadSpec small_sizes(size_t count = 500000) {
        WorkloadSpec w;
        w.name = "small_sizes";
        w.alloc_sizes.assign(count, 32); // 32B each
        w.accesses_per_alloc = 16;
        w.access_pattern = AccessType::SEQUENTIAL;
        return w;
    }

    static WorkloadSpec large_sizes(size_t count = 500) {
        WorkloadSpec w;
        w.name = "large_sizes";
        w.alloc_sizes.assign(count, 64 * 500); // 6.4KB each
        w.accesses_per_alloc = 16;
        w.access_pattern = AccessType::SEQUENTIAL;
        return w;
    }

    static WorkloadSpec mixed_sizes(size_t count = 5000) {
        WorkloadSpec w;
        w.name = "mixed_sizes";
        std::mt19937 rng(42); // fixed seed — reproducible across impls
        std::uniform_int_distribution<size_t> dist(16, 6384); // average 3.2KB
        for (size_t i = 0; i < count; ++i)
            w.alloc_sizes.push_back(dist(rng));
        w.accesses_per_alloc = 16;
        w.access_pattern = AccessType::RANDOM;
        return w;
    }

    static WorkloadSpec realloc(size_t count = 5000) {
        WorkloadSpec w;
        w.name = "realloc";
        w.alloc_sizes.assign(count, 3200); // 320B each
        w.accesses_per_alloc = 16;
        w.access_pattern = AccessType::SEQUENTIAL;
        w.include_free_cycle = true;
        return w;
    }

    static WorkloadSpec random_access(size_t count = 5000) {
        WorkloadSpec w;
        w.name = "random_access";
        w.alloc_sizes.assign(count, 3200);
        w.accesses_per_alloc = 16;
        w.access_pattern = AccessType::RANDOM;
        return w;
    }

    //4. few allocations, random accesses. (done)
    static WorkloadSpec global_random_access(size_t count = 5000) {
        WorkloadSpec w;
        w.name = "global_random_access";
        w.alloc_sizes.assign(count, 3200);
        w.accesses_per_alloc = 16;
        w.access_pattern = AccessType::RANDOM;
        w.include_global_random_access = true;
        return w;
    }

    static std::vector<WorkloadSpec> all() {
        return {
            small_sizes(),
            large_sizes(),
            mixed_sizes(),
            realloc(),
            random_access(),
            global_random_access()
        };
    }
};
*/