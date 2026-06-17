#include "memory.h"

class BasicShadow : public Memory {
public:
    BasicShadow(size_t mem_size) : Memory(mem_size) {}

    uint64_t alloc(size_t size, uint8_t tag) override {
        uint64_t addr = Memory::alloc(size, tag); // base allocator
        if (addr == INVALID) return INVALID;
        tag_range(addr, size, tag); // tag it
        return addr;
    }

    void free(uint64_t addr) override {
        size_t size = alloc_size(addr); // read size before zeroing
        tag_range(addr, size, 0); // clear tags
        Memory::free(addr); // free memory
    }

    bool check_tag(uint64_t addr, uint8_t expected) override {
        stats_.tag_accesses++;
        stats_.total_accesses++;
        return shadow_[addr] == expected;
    }

    size_t get_overhead_bytes() const override {
        return size_; // shadow memory is same size as main memory
    }

    std::string name() const override { return "BasicShadow"; }

    AccessStats get_stats() const override { return stats_; }
    void reset_stats() override { stats_ = AccessStats(); }

private:
    void tag_range(uint64_t addr, size_t size, uint8_t tag) {
        for (size_t i = 0; i < size; ++i) {
            shadow_[addr + i] = tag;
        }
    }

    uint8_t* shadow_;
};