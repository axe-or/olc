#include "testing.hpp"
#include "../base.hpp"

namespace test {

// Fresh heap over a static buffer, the buffer is dirtied first so zero-filling is actually observable
static Heap_Allocator make_test_heap(){
	alignas(64) static u8 heap_buffer[256 * 1024];
	mem_set(heap_buffer, 0xab, sizeof(heap_buffer));
	return heap_from_buffer({heap_buffer, sizeof(heap_buffer)});
}

void heap_tests(){
	test("heap: alloc and free", [](Test& t){
		Heap_Allocator heap = make_test_heap();

		u8* a = (u8*)heap.alloc({100, 8});
		u8* b = (u8*)heap.alloc({100, 8});
		check(a != nullptr && b != nullptr && a != b);
		check((uintptr)a % 8 == 0 && (uintptr)b % 8 == 0);

		bool zeroed = true;
		for(usize i = 0; i < 100; i++){ zeroed = zeroed && a[i] == 0 && b[i] == 0; }
		check(zeroed);

		for(usize align = 1; align <= 4096; align *= 2){
			void* p = heap.alloc({24, align});
			check(p != nullptr && (uintptr)p % align == 0);
			heap.free(p);
		}

		heap.free(a);
		heap.free(b);

		// Everything was freed, the whole pool should be usable again
		void* big = heap.alloc({200 * 1024, 16});
		check(big != nullptr);
		heap.free(big);

		check(heap.alloc({1024 * 1024, 16}) == nullptr);
	});

	test("heap: realloc", [](Test& t){
		Heap_Allocator heap = make_test_heap();

		u8* p = (u8*)heap.alloc({16, 8});
		for(usize i = 0; i < 16; i++){ p[i] = u8(i + 1); }

		// Same alignment
		u8* grown = (u8*)heap.realloc(p, {16, 8}, {4096, 8});
		check(grown != nullptr && (uintptr)grown % 8 == 0);
		bool kept = true;
		for(usize i = 0; i < 16; i++){ kept = kept && grown[i] == u8(i + 1); }
		check(kept);
		bool zeroed = true;
		for(usize i = 16; i < 4096; i++){ zeroed = zeroed && grown[i] == 0; }
		check(zeroed);

		// Different alignment
		u8* aligned = (u8*)heap.realloc(grown, {4096, 8}, {8192, 256});
		check(aligned != nullptr && (uintptr)aligned % 256 == 0);
		kept = true;
		for(usize i = 0; i < 16; i++){ kept = kept && aligned[i] == u8(i + 1); }
		check(kept);
		zeroed = true;
		for(usize i = 4096; i < 8192; i++){ zeroed = zeroed && aligned[i] == 0; }
		check(zeroed);

		u8* shrunk = (u8*)heap.realloc(aligned, {8192, 256}, {8, 256});
		check(shrunk != nullptr && (uintptr)shrunk % 256 == 0);
		kept = true;
		for(usize i = 0; i < 8; i++){ kept = kept && shrunk[i] == u8(i + 1); }
		check(kept);
		heap.free(shrunk);
	});

	test("heap: allocator interface", [](Test& t){
		Heap_Allocator heap = make_test_heap();
		Allocator allocator = heap.allocator();

		check(allocator.query() == (Mem_Alloc | Mem_Grow | Mem_Shrink | Mem_Free));

		u32* nums = (u32*)allocator.alloc(layout_of<u32>(4));
		check(nums != nullptr && nums[0] == 0 && nums[3] == 0);
		for(u32 i = 0; i < 4; i++){ nums[i] = i * 10; }

		nums = (u32*)allocator.grow(nums, layout_of<u32>(4), layout_of<u32>(64));
		check(nums != nullptr && nums[3] == 30 && nums[4] == 0 && nums[63] == 0);

		nums = (u32*)allocator.shrink(nums, layout_of<u32>(64), layout_of<u32>(2));
		check(nums != nullptr && nums[0] == 0 && nums[1] == 10);

		allocator.free(nums, layout_of<u32>(2));
	});
}

}
