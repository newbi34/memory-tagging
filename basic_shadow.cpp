#include "memory.h"

class BasicShadow : public Memory { // 5. granule size approach (done)
public:
    BasicShadow(size_t mem_size, size_t granule_size)
        : Memory(mem_size),
          granule_size_(granule_size),
          shadow_size_(mem_size / granule_size),
          shadow_(new uint8_t[mem_size / granule_size]()) {}

    ~BasicShadow() {
        delete[] shadow_;
    }

    uint64_t alloc(size_t size, uint8_t tag) override {
        uint64_t addr = Memory::alloc(size, tag);
        if (addr == INVALID) return INVALID;
        tag_range(addr, size, tag);

        shadow_ranges_.push_back({addr, size});

        stats_.peak_overhead_bytes = std::max(stats_.peak_overhead_bytes, get_overhead_bytes());

        return addr;
    }

    void free(uint64_t addr) override {
        size_t size = alloc_size(addr);
        tag_range(addr, size, 0);
        Memory::free(addr);
    }

    bool check_tag(uint64_t addr, uint8_t expected) override {
        stats_.tag_accesses++;
        stats_.bytes_transferred++;

        uint64_t shadow_index = addr / granule_size_;
        if (shadow_index >= shadow_size_) return false;

        return shadow_[shadow_index] == expected;
    }

    size_t get_overhead_bytes() override {
        uint64_t overhead = 0;

        for (const auto& range : shadow_ranges_) {
            overhead += granule_size_ - (range.second % granule_size_); // padding for last granule
        }

        return shadow_size_ + overhead;
    }

    std::string name() const override {
        return "BasicShadow(g=" + std::to_string(granule_size_) + ")";
    }

    AccessStats get_stats() const override { return stats_; }
    void reset_stats() override { stats_ = AccessStats(); }

    void print_overhead_bytes() override {
        std::cout << "Overhead bytes: " << get_overhead_bytes()
                  << " (1 byte per " << granule_size_ << "B granule)\n";
    }

private:
    void tag_range(uint64_t addr, size_t size, uint8_t tag) {
        // first granule that overlaps with [addr, addr+size)
        uint64_t first_granule = addr / granule_size_;

        // last granule that overlaps - round up
        uint64_t last_granule  = (addr + size - 1) / granule_size_;

        for (uint64_t g = first_granule; g <= last_granule; ++g) {
            if (g >= shadow_size_) break;
            shadow_[g] = tag;
            stats_.tag_accesses++;
            stats_.bytes_transferred++;
        }
    }

    // we dont consider looking up grandule/shadow size as memory access since its a constant and can be hardcoded in hardware
    size_t   granule_size_;
    size_t   shadow_size_;  // number of granules = mem_size / granule_size
    std::vector<std::pair<uint64_t, uint64_t>> shadow_ranges_; // for overhead, not used in stats

    uint8_t* shadow_;
};