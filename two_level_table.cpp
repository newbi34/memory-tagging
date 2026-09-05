#include "memory.h"
#include <cassert>

class TwoLevelTable : public Memory {
public:
    TwoLevelTable(size_t mem_size,
                  size_t page_size   = 4096,
                  size_t granule_size = 16)
        : Memory(mem_size, granule_size),
          page_size_(page_size),
          granule_size_(granule_size),
          granules_per_page_(page_size / granule_size),
                    num_pages_((mem_size + page_size - 1) / page_size),
                    table_(new uint8_t*[(mem_size + page_size - 1) / page_size]()) {

        // sanity checks
                assert(page_size > 0 && "page size must be > 0");
                assert(granule_size > 0 && "granule size must be > 0");
        assert(page_size >= granule_size && "page must be larger than granule");
        assert(page_size % granule_size == 0 && "page must be multiple of granule");
    }

    ~TwoLevelTable() {
        for (size_t i = 0; i < num_pages_; ++i)
            delete[] table_[i];
        delete[] table_;
    }

    void reset_shadow() {
        for (size_t i = 0; i < num_pages_; ++i) {
            delete[] table_[i];
            table_[i] = nullptr;
        }
        shadow_ranges_.clear();
    }

    uint64_t alloc(size_t size, uint8_t tag) override {
        uint64_t addr = Memory::alloc(size, tag);
        if (addr == INVALID) return INVALID;
        tag_range(addr, size, tag);

        shadow_ranges_.push_back({addr, size});

        size_t current = get_overhead_bytes();
        if (current > stats_.peak_overhead_bytes)
            stats_.peak_overhead_bytes = current;

        return addr;
    }

    void free(uint64_t addr) override {
        size_t size = alloc_size(addr);
        tag_range(addr, size, 0);

        // Remove the tracking entry for this address
        for (auto it = shadow_ranges_.begin(); it != shadow_ranges_.end(); ++it) {
            if (it->first == addr) {
                shadow_ranges_.erase(it);
                break;
            }
        }

        Memory::free(addr);
    }

    bool check_tag(uint64_t addr, uint8_t expected) override {
        uint64_t page_index = addr / page_size_;
        uint64_t offset     = (addr % page_size_) / granule_size_;

        if (page_index >= num_pages_)
            return false;

        if (table_[page_index] == nullptr)
            return expected == 0; // unallocated page — treat as untagged

        stats_.tag_accesses++;
        stats_.pointer_accesses++;
        stats_.bytes_transferred++;

        return table_[page_index][offset] == expected;
    }

    size_t get_overhead_bytes() override {
        size_t overhead = num_pages_ * sizeof(uint8_t*); // level 1
        for (size_t i = 0; i < num_pages_; ++i)
            if (table_[i])
                overhead += granules_per_page_ * sizeof(uint8_t); // level 2

        for (const auto& range : shadow_ranges_) {
            if (range.second % granule_size_ != 0) {
                overhead += granule_size_ - (range.second % granule_size_); // padding for last granule
            }
        }
            
        return overhead;
    }

    void print_overhead_bytes() override {
        size_t l1 = num_pages_ * sizeof(uint8_t*);
        size_t l2 = 0;
        for (size_t i = 0; i < num_pages_; ++i)
            if (table_[i]) l2 += granules_per_page_;
        std::cout << std::dec
                  << "Page size       : " << page_size_    << " bytes\n"
                  << "Granule size    : " << granule_size_ << " bytes\n"
                  << "Granules/page   : " << granules_per_page_ << "\n";

            size_t table_overhead = num_pages_ * sizeof(uint8_t*);
            for (size_t i = 0; i < num_pages_; ++i)
                if (table_[i])
                    table_overhead += granules_per_page_ * sizeof(uint8_t);

            size_t padding_overhead = 0;
            for (const auto& range : shadow_ranges_) {
                if (range.second % granule_size_ != 0) {
                    padding_overhead += granule_size_ - (range.second % granule_size_);
                }
            }

            std::cout << "Table structure: " << table_overhead 
                      << " B | Padding: " << padding_overhead 
                      << " B | Range count: " << shadow_ranges_.size() << "\n";
    }

    std::string name() const override {
        return "TwoLevelTable(p=" + std::to_string(page_size_) + "g=" + std::to_string(granule_size_) + ")";
    }

    AccessStats get_stats()  const override { return stats_; }
    void reset_stats() override { stats_ = AccessStats(); }

private:
    static uint64_t align_down(uint64_t value, uint64_t alignment) {
        return (value / alignment) * alignment;
    }

    static uint64_t align_up(uint64_t value, uint64_t alignment) {
        return ((value + alignment - 1) / alignment) * alignment;
    }

    void tag_range(uint64_t addr, size_t size, uint8_t tag) {
        // align down to granule boundary and cover all touched granules
        uint64_t granule_start = align_down(addr, granule_size_);
        uint64_t granule_end   = align_up(addr + size, granule_size_);

        for (uint64_t cur = granule_start; cur < granule_end; cur += granule_size_) {
            uint64_t page_index = cur / page_size_;
            uint64_t offset     = (cur % page_size_) / granule_size_;

            if (page_index >= num_pages_) break;

            if (!table_[page_index])
                table_[page_index] = new uint8_t[granules_per_page_]();

            table_[page_index][offset] = tag;

            stats_.bytes_transferred++;
            stats_.tag_accesses++;
            stats_.pointer_accesses++;
        }
    }

    size_t page_size_;
    size_t granule_size_;
    size_t granules_per_page_;
    size_t num_pages_;
    std::vector<std::pair<uint64_t, uint64_t>> shadow_ranges_; // for overhead, not used in stats

    uint8_t** table_;
};