#pragma once

#include <cstdint>
#include <string>

class ITaggingImpl {
public:
    virtual ~ITaggingImpl() = default;

    virtual void  tag_alloc(uint64_t addr, size_t size, uint8_t tag) = 0;
    virtual bool  check_tag(uint64_t addr, uint8_t expected_tag) = 0;
    virtual void  free_alloc(uint64_t addr) = 0;
    virtual size_t get_overhead_bytes() const  = 0;
    virtual std::string name() const  = 0;
};