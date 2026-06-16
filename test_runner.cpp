#include <iostream>
#include "two_level_table.cpp"

int main() {
    TwoLevelTable memory(1024 * 1024); // 1MB memory

    uint64_t ptr = memory.alloc(4, 5); // allocate 4 bytes
    std::cout << "Allocated at: 0x" << std::hex << ptr << std::endl;
    const uint8_t data[4] = {0xFF, 0xEE, 0xDD, 0xCC}; // sample data
    memory.write(ptr, data, 4); // write data
    uint64_t addr = ptr;

    if (addr) {
        if (memory.check_tag(addr, 5)) {
            std::cout << "Tag check passed! value: 0x" << static_cast<int>(memory.read(addr)[0]) << std::endl;
        } else {
            std::cout << "Tag check failed!" << std::endl;
        }

        if (memory.check_tag(addr, 6)) {
            std::cout << "Tag check passed with wrong tag! value: 0x" << static_cast<int>(memory.read(addr)[0]) << std::endl;
        } else {
            std::cout << "Tag check correctly failed with wrong tag!" << std::endl;
        }

        if (memory.check_tag(addr + 12, 5)) {
            std::cout << "Tag check passed for next granule! value: 0x" << static_cast<int>(memory.read(addr + 12)[0]) << std::endl;
        } else {
            std::cout << "Tag check failed for next granule!" << std::endl;
        }

        memory.print_overhead_bytes(); // print overhead

        memory.free(addr); // free allocation

        memory.print_stats(); // print stats after free
    }

    return 0;
}