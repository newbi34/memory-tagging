#include "memory.h"
#include <cstdint>
#include <string>
#include <vector>
#include <cstring>

Memory::Memory(size_t size, size_t granule_size) : buffer_(size, 0), size_(size), allocated_(size, false), granule_size_(granule_size) {}

uint64_t Memory::alloc(size_t size, uint8_t tag) {
    uint64_t addr = find_free_block(size);
    if (addr == INVALID) return INVALID;

    uint32_t size_header = static_cast<uint32_t>(size);
    std::memcpy(&buffer_[addr], &size_header, sizeof(size_header));

    // total bytes to mark = header + size, rounded up to granule boundary
    size_t total = (sizeof(uint32_t) + size + granule_size_ - 1) & ~size_t(granule_size_ - 1);
    for (size_t i = 0; i < total; ++i)
        allocated_[addr + i] = true;

    stats_.alloc_count++;
    stats_.bytes_allocated += size;

    return addr + sizeof(uint32_t);
}

void Memory::free(uint64_t addr) {
    uint64_t block = addr - sizeof(uint32_t);
    uint32_t alloc_size = 0;
    std::memcpy(&alloc_size, &buffer_[block], sizeof(alloc_size));

    size_t total = (sizeof(uint32_t) + alloc_size + granule_size_ - 1) & ~size_t(granule_size_ - 1);
    std::memset(&buffer_[block], 0, total);
    for (size_t i = 0; i < total; ++i)
        allocated_[block + i] = false;

    stats_.free_count++;
    stats_.bytes_allocated -= alloc_size;
}

void Memory::write(uint64_t addr, const uint8_t* data, size_t size) {
    if (addr + size > size_) return;
    stats_.simulated_accesses++; 
    std::memcpy(&buffer_[addr], data, size);
}

uint8_t* Memory::read(uint64_t addr) {
    if (addr >= size_) return nullptr;
    stats_.simulated_accesses++;
    return &buffer_[addr];
}

void Memory::print_stats() {
    auto s = get_stats();
    std::cout << "[" << name() << "]\n"
          << "  tag accesses    : " << s.tag_accesses    << "(times gone to memory to write or read tags)" <<"\n"
          << "  pointer accesses: " << s.pointer_accesses << "(times gone to memory to write or read pointers)" << "\n"
          << "  bytes transferred: " << s.bytes_transferred << "(total bytes transferred in real memory accesses)" << "\n"
          << "  simulated accesses: " << s.simulated_accesses << "(simulated memory accesses)" << "\n"
          << "  alloc count     : " << s.alloc_count     << "\n"
          << "  free count      : " << s.free_count      << "\n"
          << "  bytes allocated : " << s.bytes_allocated << "\n"
          << "  overhead bytes  : " << get_overhead_bytes() << "\n";
}

size_t Memory::alloc_size(uint64_t addr) const {
    uint32_t alloc_size = 0;
    std::memcpy(&alloc_size, &buffer_[addr - sizeof(uint32_t)], sizeof(alloc_size));
    return alloc_size;
}

uint64_t Memory::find_free_block(size_t size) {
    size_t needed = (sizeof(uint32_t) + size + granule_size_ - 1) & ~size_t(granule_size_ - 1); // round up to granule
    size_t run = 0;
    size_t run_start = 0;

    for (size_t i = 0; i < size_; i += granule_size_) {
        if (!allocated_[i]) {
            if (run == 0) run_start = i; // mark where run began
            run += granule_size_;
            if (run >= needed)
                return run_start; // always return start of run
        } else {
            run = 0; // reset - occupied chunk
        }
    }
    return INVALID;
}