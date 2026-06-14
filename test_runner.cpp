#include <vector>
#include <cstdint>
#include "two_level_table.cpp"

class Memory {
public:
    Memory(size_t size) : mem_(size, 0) {}

    uint8_t* allocate(uint64_t addr, size_t size) {
        if (size > mem_.size()) return nullptr; // not enough memory
        return mem_.data() + addr; // return pointer to start of chunk (for simplicity, no actual allocation logic)
    }

private:
    std::vector<uint8_t> mem_;
};

int main() {
    Memory mem(1024 * 1024); // 1MB memory
    TwoLevelTable tag_table(1024 * 1024); // support for 1MB memory

    uint8_t* ptr = mem.allocate(64, 64); // allocate 64 bytes
    ptr[0] = 0xFF; // write to allocated memory
    uint64_t addr = 64;
    if (addr) {
        tag_table.tag_alloc(addr, 64, 0xAB); // tag with 0xAB

        if (tag_table.check_tag(addr, 0xAB)) {
            printf("Tag check passed! value: %02X\n", ptr[0]);
        } else {
            printf("Tag check failed!\n");
        }

        if (tag_table.check_tag(addr, 0xCD)) {
            printf("Tag check passed with wrong tag! value: %02X\n", ptr[0]);
        } else {
            printf("Tag check correctly failed with wrong tag!\n");
        }

        if (tag_table.check_tag(addr + 16, 0xAB)) {
            printf("Tag check passed for next granule! value: %02X\n", ptr[16]);
        } else {
            printf("Tag check failed for next granule!\n");
        }
        
        tag_table.free_alloc(addr); // free allocation
    }

    return 0;
}