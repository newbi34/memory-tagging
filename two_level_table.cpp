#include "structure_interface.h"
#include <vector>
#include <unordered_map>

// Default tag is 0, means untagged memory. Each tag is 1 byte big and is valid for 16 bytes, so we can store 256 tags in a 4KB page.

class TwoLevelTable : ITaggingImpl {
public:
    TwoLevelTable(uint64_t mem_size) {
        table_.resize(mem_size / 4096); // 4KB pages
    }

    ~TwoLevelTable() {
        table_.clear();
    }

    void tag_alloc(uint64_t addr, size_t size, uint8_t tag) override {
        alloc_sizes_[addr] = size;  

        size_t granules = (size + 15) / 16; // round up
        for (size_t i = 0; i < granules; ++i) {
            uint64_t cur_addr = addr + i * 16;
            uint64_t page_index = cur_addr / 4096;
            uint64_t offset = (cur_addr % 4096) / 16;   

            if (page_index >= table_.size()) break; // out of covered range 

            if (table_[page_index].empty())
                table_[page_index].resize(256, 0);  

            table_[page_index][offset] = tag;
        }
        alloc_sizes_[addr] = size; // store size for free
    }

    bool check_tag(uint64_t addr, uint8_t expected_tag) override {
        uint64_t page_index = addr / 4096;
        uint64_t offset = (addr % 4096) / 16;

        if (page_index >= table_.size() || table_[page_index].empty()) {
            return false;
        }

        return table_[page_index][offset] == expected_tag; // single granule
    }

    void free_alloc(uint64_t addr) override {
        auto it = alloc_sizes_.find(addr);
        if (it == alloc_sizes_.end()) return; // unknown allocation

        size_t size = it->second;
        uint64_t page_index = addr / 4096;
        uint64_t offset = (addr % 4096) / 16;

        size_t granules = (size + 15) / 16; // round up to granule boundary
        for (size_t i = 0; i < granules; ++i) {
            uint64_t cur_page = (addr + i * 16) / 4096;
            uint64_t cur_offset = ((addr + i * 16) % 4096) / 16;
            if (cur_page < table_.size() && !table_[cur_page].empty()) {
                table_[cur_page][cur_offset] = 0; // zero = untagged
            }
        }
        alloc_sizes_.erase(it);
    }  

    size_t get_overhead_bytes() const override {
        size_t overhead = 0;
        overhead += table_.capacity() * sizeof(std::vector<uint8_t>); // level 1
        for (const auto& page : table_) {
            if (!page.empty()) {
                overhead += 256; // level 2 arrays
            }
        }
        overhead += alloc_sizes_.size() * (sizeof(uint64_t) + sizeof(size_t)); // metadata map
        return overhead;
    }

    std::string name() const override { return "TwoLevelTable"; }

private:
    std::vector<std::vector<uint8_t>> table_;
    std::unordered_map<uint64_t, size_t> alloc_sizes_; // addr to size
};