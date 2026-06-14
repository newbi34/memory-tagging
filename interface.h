#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

class ITaggingImpl {
public:
    virtual ~ITaggingImpl() = default;

    virtual void  tag_alloc(void* ptr, size_t size, uint8_t tag) = 0;
    virtual bool  check_tag(void* ptr, uint8_t expected_tag) = 0;
    virtual void  free_alloc(void* ptr) = 0;
    virtual size_t get_overhead_bytes() const  = 0;
    virtual std::string name() const  = 0;
};