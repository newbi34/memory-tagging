#include "memory.h"
#include <cstring>
#include <vector>

struct CuckooEntry {
    uint64_t addr = 0;   // granule-aligned address
    uint8_t  tag  = 0;
    bool used = false;
};

class CuckooHashTable : public Memory {
public:
    CuckooHashTable(size_t mem_size, size_t table_size = 1024)
        : Memory(mem_size),
          table_size_(round_up_pow2(table_size)),
          tableA_(new CuckooEntry[table_size_]()),
          tableB_(new CuckooEntry[table_size_]()) {}

    ~CuckooHashTable() {
        delete[] tableA_;
        delete[] tableB_;
    }

    uint64_t alloc(size_t size, uint8_t tag) override {
        uint64_t addr = Memory::alloc(size, tag);
        if (addr == INVALID) return INVALID;

        if (!tag_range(addr, size, tag)) {
            Memory::free(addr);
            return INVALID;
        }

        // update peak after every insert
        size_t current = get_overhead_bytes();
        if (current > stats_.peak_overhead_bytes)
            stats_.peak_overhead_bytes = current;

        return addr;
    }

    void free(uint64_t addr) override {
        size_t sz = alloc_size(addr);
        clear_range(addr, sz);
        Memory::free(addr);
    }

    bool check_tag(uint64_t addr, uint8_t expected) override {
        stats_.tag_accesses++;
        stats_.total_accesses++;

        uint64_t granule = (addr - sizeof(uint32_t)) & ~0xFULL; // align to 16B granule
        CuckooEntry* e = lookup(granule);
        if (!e) return false;

        return e->tag == expected;
    }

    size_t get_overhead_bytes() override {
        return 2 * table_size_ * sizeof(CuckooEntry);
    }

    void print_overhead_bytes() {
        std::cout << "Overhead bytes: " << get_overhead_bytes() << "\n";
    }

    std::string name() const override { return "CuckooHashTable"; }
    AccessStats get_stats() const override { return stats_; }
    void reset_stats() override { stats_ = AccessStats(); }

private:
    static constexpr int MAX_EVICTIONS = 32;

    size_t table_size_;
    CuckooEntry* tableA_;
    CuckooEntry* tableB_;
    int rehash_count_ = 0;

    size_t h1(uint64_t granule_addr) const {
        uint64_t k = (granule_addr >> 4); // granule index
        k ^= (uint64_t)rehash_count_ * 0xdeadbeefULL;
        return (k * 0x9e3779b97f4a7c15ULL) >> (64 - index_bits());
    }

    size_t h2(uint64_t granule_addr) const {
        uint64_t k = (granule_addr >> 4);
        k ^= (uint64_t)rehash_count_ * 0xcafebabeULL;
        return (k * 0x6c62272e07bb0142ULL) >> (64 - index_bits());
    }

    size_t index_bits() const {
        size_t bits = 0;
        size_t n = table_size_;
        while (n > 1) { n >>= 1; ++bits; }
        return bits;
    }

    static size_t round_up_pow2(size_t n) {
        size_t p = 1;
        while (p < n) p <<= 1;
        return p;
    }

    CuckooEntry* lookup(uint64_t granule_addr) {
        size_t sa = h1(granule_addr);

        stats_.tag_accesses += 2;
        stats_.total_accesses += 2;

        if (tableA_[sa].used && tableA_[sa].addr == granule_addr)
            return &tableA_[sa];

        stats_.tag_accesses += 2;
        stats_.total_accesses += 2;

        size_t sb = h2(granule_addr);
        if (tableB_[sb].used && tableB_[sb].addr == granule_addr)
            return &tableB_[sb];

        return nullptr;
    }

    bool insert(uint64_t granule_addr, uint8_t tag) {
        CuckooEntry* existing = lookup(granule_addr);
        if (existing) {
            existing->tag = tag;
            return true;
        }

        CuckooEntry displaced = {granule_addr, tag, true};

        for (int i = 0; i < MAX_EVICTIONS; ++i) {
            size_t sa = h1(displaced.addr);

            stats_.tag_accesses++;
            stats_.total_accesses++;
            if (!tableA_[sa].used) {
                stats_.tag_accesses++;
                stats_.total_accesses++;
                tableA_[sa] = displaced;
                return true;
            }
            std::swap(tableA_[sa], displaced);

            stats_.tag_accesses++;
            stats_.total_accesses++;

            size_t sb = h2(displaced.addr);
            if (!tableB_[sb].used) {
                stats_.tag_accesses++;
                stats_.total_accesses++;
                tableB_[sb] = displaced;
                return true;
            }
            std::swap(tableB_[sb], displaced);
        }

        if (!rehash()) return false;
        return insert(displaced.addr, displaced.tag);
    }

    void remove(uint64_t granule_addr) {
        size_t sa = h1(granule_addr);
        stats_.tag_accesses += 2;
        stats_.total_accesses += 2;
        if (tableA_[sa].used && tableA_[sa].addr == granule_addr) {
            stats_.tag_accesses++;
            stats_.total_accesses++;
            tableA_[sa] = CuckooEntry{};
            return;
        }

        size_t sb = h2(granule_addr);
        stats_.tag_accesses += 2;
        stats_.total_accesses += 2;
        if (tableB_[sb].used && tableB_[sb].addr == granule_addr) {
            stats_.tag_accesses++;
            stats_.total_accesses++;
            tableB_[sb] = CuckooEntry{};
        }
    }

    bool tag_range(uint64_t addr, size_t size, uint8_t tag) {
        size_t granules = (size + 15 + sizeof(uint32_t)) / 16;
        for (size_t i = 0; i < granules; ++i) {
            uint64_t g = (addr + i * 16) & ~0xFULL;
            if (!insert(g, tag)) return false;
        }
        return true;
    }

    void clear_range(uint64_t addr, size_t size) {
        size_t granules = (size + 15) / 16;
        for (size_t i = 0; i < granules; ++i) {
            uint64_t g = (addr + i * 16) & ~0xFULL;
            remove(g);
        }
    }

    bool rehash() {
        std::vector<CuckooEntry> live;
        live.reserve(table_size_);
        for (size_t i = 0; i < table_size_; ++i) {
            if (tableA_[i].used) live.push_back(tableA_[i]);
            if (tableB_[i].used) live.push_back(tableB_[i]);
        }

        size_t new_size = table_size_ * 2;
        delete[] tableA_;
        delete[] tableB_;
        tableA_ = new CuckooEntry[new_size]();
        tableB_ = new CuckooEntry[new_size]();
        table_size_ = new_size;
        rehash_count_++;

        for (auto& e : live) {
            if (!insert(e.addr, e.tag))
                return false;
        }
        return true;
    }

    size_t count_entries() {
        size_t count = 0;
        for (size_t i = 0; i < table_size_; ++i) {
            stats_.tag_accesses += 2;
            stats_.total_accesses += 2;
            if (tableA_[i].used) ++count;
            if (tableB_[i].used) ++count;
        }
        return count;
    }
};