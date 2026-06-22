#include <iostream>
#include <string>
#include "two_level_table.cpp"
#include "balanced_tree.cpp"
#include "hash_table.cpp"
#include "basic_shadow.cpp"

static int tests_run = 0;
static int tests_passed = 0;

void check(const std::string &label, bool result, bool expected)
{
	tests_run++;
	bool pass = (result == expected);
	if (pass)
		tests_passed++;
	std::cout << (pass ? "[PASS] " : "[FAIL] ") << label << "\n";
}

void print_summary()
{
	std::cout << "\n" << tests_passed << "/" << tests_run << " tests passed\n";
	tests_run = tests_passed = 0; // reset for next implementation
}

void run_benchmark_tests(Memory &mem)
{
	
}

void run_correctness_tests(Memory &mem)
{
	std::cout << "\n=== " << mem.name() << " ===\n\n";

	// --- Basic tag match ---
	uint64_t ptr = mem.alloc(64, 0xA);
	check("alloc returns valid address", ptr != ~0ULL, true);

	check("correct tag accepted", mem.check_tag(ptr, 0xA), true);

	check("wrong tag rejected", mem.check_tag(ptr, 0xB), false);

	// --- Granule coverage ---
	// allocation is 64 bytes = 4 granules, check last granule (ptr+48)
	check("tag valid on last granule of allocation", mem.check_tag(ptr + 48, 0xA), true);

	check("tag invalid just outside allocation", mem.check_tag(ptr + 64, 0xA), false);

	// --- Write and read ---
	const uint8_t data[4] = {0xFF, 0xEE, 0xDD, 0xCC};
	mem.write(ptr, data, 4);
	uint8_t *buf = mem.read(ptr);
	check("write then read returns correct value", buf != nullptr && buf[0] == 0xFF, true);

	// --- Use after free ---
	mem.free(ptr);
	check("tag cleared after free", mem.check_tag(ptr, 0xA), false);

	// --- Adjacent allocations don't bleed ---
	uint64_t a = mem.alloc(32, 0x1);
	uint64_t b = mem.alloc(32, 0x2);
	check("adjacent alloc A has correct tag", mem.check_tag(a, 0x1), true);
	check("adjacent alloc B has correct tag", mem.check_tag(b, 0x2), true);
	check("alloc B address rejected for alloc A tag", mem.check_tag(b, 0x1), false);
	check("alloc A address rejected for alloc B tag", mem.check_tag(a, 0x2), false);
	mem.free(a);
	mem.free(b);

	// --- Multiple alloc/free cycles ---
	uint64_t c = mem.alloc(16, 0x5);
	mem.free(c);
	uint64_t d = mem.alloc(16, 0x6); // should reuse c's space
	check("tag correct after realloc of same region", mem.check_tag(d, 0x6), true);
	check("old tag rejected after realloc", mem.check_tag(d, 0x5), false);
	mem.free(d);

	// --- Stats and overhead ---
	mem.print_stats();
	mem.print_overhead_bytes();
	print_summary();
}

int main()
{
	{
		TwoLevelTable mem(1024 * 1024);
		run_correctness_tests(mem);
	}
	{
		BalancedTree mem(1024 * 1024);
		run_correctness_tests(mem);
	}
	{
		CuckooHashTable mem(1024 * 1024);
		run_correctness_tests(mem);
	}
	{
		BasicShadow mem(1024 * 1024);
		run_correctness_tests(mem);
	}
	return 0;
}