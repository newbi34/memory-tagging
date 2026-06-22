#include "memory.h"

class TwoLevelTable : public Memory {
public:
    TwoLevelTable(size_t mem_size)
        : Memory(mem_size),
          num_pages_(mem_size / 4096),
          table_(new uint8_t*[mem_size / 4096]()) {}

    ~TwoLevelTable() {
        for (size_t i = 0; i < num_pages_; ++i)
            delete[] table_[i];
        delete[] table_;
    }

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
        uint64_t page_index = addr / 4096;
        uint64_t offset = (addr % 4096) / 16;
        if (page_index >= num_pages_ || table_[page_index] == nullptr)
            return false;
        stats_.tag_accesses++;
        stats_.total_accesses++;
        return table_[page_index][offset] == expected;
    }

    size_t get_overhead_bytes() override {
        size_t overhead = num_pages_ * sizeof(uint8_t*);
        for (size_t i = 0; i < num_pages_; ++i)
            if (table_[i]) overhead += 256;
        return overhead;
    }

    void print_overhead_bytes() {
        size_t overhead1 = num_pages_ * sizeof(uint8_t*);
        size_t overhead2 = 0;
        for (size_t i = 0; i < num_pages_; ++i)
            if (table_[i]) overhead2 += 256;
        std::cout << std::dec << "Level 1 table bytes: " << overhead1 << std::endl;
        std::cout << std::dec << "Level 2 table bytes: " << overhead2 << std::endl;
        std::cout << std::dec << "Total overhead bytes: " << overhead1 + overhead2 << std::endl;
    }

    std::string name() const override { return "TwoLevelTable"; }
    AccessStats get_stats() const override { return stats_; }
    void reset_stats() override { stats_ = AccessStats(); }

private:
    void tag_range(uint64_t addr, size_t size, uint8_t tag) {
        size_t granules = (size + 15 + sizeof(uint32_t)) / 16;
        uint64_t granule_start = (addr - sizeof(uint32_t)) & ~static_cast<uint64_t>(15);
        for (size_t i = 0; i < granules; ++i) {
            uint64_t cur = granule_start + i * 16;
            uint64_t page_index = cur / 4096;
            uint64_t offset = (cur % 4096) / 16;
            if (page_index >= num_pages_) break;
            if (!table_[page_index])
                table_[page_index] = new uint8_t[256]();
            table_[page_index][offset] = tag;
        }
        stats_.tag_accesses += 2 * granules;
        stats_.total_accesses += 2 * granules;
    }

    size_t num_pages_;
    uint8_t** table_;
};