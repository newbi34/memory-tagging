#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <cstring>

class AccessStats
{
public:
    uint64_t total_accesses = 0;
    uint64_t tag_accesses = 0;
    uint64_t alloc_count = 0;
    uint64_t free_count = 0;
    uint64_t bytes_allocated = 0;
};

class ITaggingImpl
{
public:
    virtual ~ITaggingImpl() = default;
    virtual uint64_t alloc(size_t size, uint8_t tag) = 0;
    virtual void free(uint64_t addr) = 0;
    virtual bool check_tag(uint64_t addr, uint8_t expected) = 0;
    virtual size_t get_overhead_bytes() const = 0;
    virtual std::string name() const = 0;
    virtual AccessStats get_stats() const = 0;
    virtual void reset_stats() = 0;
};

class Memory : public ITaggingImpl
{
public:
    explicit Memory(size_t size);
    uint64_t alloc(size_t size, uint8_t tag) override;
    void free(uint64_t addr) override;
    void write(uint64_t addr, const uint8_t *data, size_t size);
    uint8_t *read(uint64_t addr);
    void print_stats() const;
    
    AccessStats get_stats() const override { return stats_; }
    void reset_stats() override { stats_ = AccessStats(); }
    bool check_tag(uint64_t, uint8_t) override { return true; }
    size_t get_overhead_bytes() const override { return 0; }
    std::string name() const override { return "Memory"; }

protected:
    static constexpr uint64_t INVALID = ~0ULL;
    size_t alloc_size(uint64_t addr) const;

    std::vector<uint8_t> buffer_;
    std::vector<bool> allocated_;
    size_t size_;
    AccessStats stats_;

private:
    uint64_t find_free_block(size_t size);
};