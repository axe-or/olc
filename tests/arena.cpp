#include "testing.hpp"
#include "../base.hpp"

static_assert(__is_aggregate(Arena));
static_assert(__is_trivially_copyable(Arena) && __is_trivially_constructible(Arena));

namespace test {

void arena_tests(){
	test("arena: alloc, resize, realloc", [](Test& t){
		alignas(64) u8 storage[513];
		mem_set(storage, 0xff, sizeof(storage));
		Arena a = arena_from_buffer(storage + 1, 512);

		check(a.owns(storage + 1));
		check(!a.owns(storage));
		check(!a.owns(storage + 513));
		check(!a.owns(nullptr));
		check(a.alloc(0, 1) == nullptr);
		check(a.alloc(1, 1) == storage + 1);

		u8* p = (u8*)a.alloc(32, 64);
		check(p != nullptr && (uintptr)p % 64 == 0);
		bool zeroed = true;
		for(usize i = 0; i < 32; i++){ zeroed = zeroed && p[i] == 0; p[i] = 42; }
		check(zeroed);

		check(a.resize(p, 64));
		bool kept = true;
		for(usize i = 0; i < 64; i++){ kept = kept && p[i] == (i < 32 ? 42 : 0); }
		check(kept);

		check(a.resize(p, 16));
		check(a.resize(p, 32));
		zeroed = true;
		for(usize i = 16; i < 32; i++){ zeroed = zeroed && p[i] == 0; }
		check(zeroed);

		usize offset = a.offset;
		check(!a.resize(p, SIZE_MAX));
		check(a.offset == offset && a.last_allocation_size == 32);
		check(a.alloc(SIZE_MAX, 1) == nullptr);
		check(a.offset == offset && a.last_allocation == p);

		// No longer the last allocation, so it can't be resized in place
		check(a.alloc(1, 1) != nullptr);
		check(!a.resize(p, 64));
		u8* moved = (u8*)a.realloc(p, 32, 64, 64);
		check(moved != nullptr && moved != p && (uintptr)moved % 64 == 0);
		kept = true;
		for(usize i = 0; i < 64; i++){ kept = kept && moved[i] == (i < 16 ? 42 : 0); }
		check(kept);

		check(a.realloc(moved, 64, 80, 64) == moved);
		zeroed = true;
		for(usize i = 64; i < 80; i++){ zeroed = zeroed && moved[i] == 0; }
		check(zeroed);

		offset = a.offset;
		check(a.realloc(p, 32, SIZE_MAX, 64) == nullptr);
		check(a.offset == offset && p[0] == 42);
	});

	test("arena: reset", [](Test& t){
		alignas(64) u8 storage[513];
		mem_set(storage, 0xff, sizeof(storage));
		Arena a = arena_from_buffer(storage + 1, 512);
		u8* p = (u8*)a.alloc(32, 1);

		a.reset();
		check(a.offset == 0 && a.last_allocation == nullptr && a.last_allocation_size == 0);
		check(!a.resize(p, 8));
		check(a.alloc(512, 1) == storage + 1);
		check(a.alloc(1, 1) == nullptr);

		bool zeroed = true;
		for(usize i = 1; i < sizeof(storage); i++){ zeroed = zeroed && storage[i] == 0; }
		check(zeroed);
		check(storage[0] == 0xff);

		a.reset();
		check(a.realloc(nullptr, 0, 16, 1) == storage + 1);
		check(a.resize(storage + 1, 0));
		check(a.offset == 0);
		check(a.alloc(512, 1) == storage + 1);
	});

	test("arena: placement new", [](Test& t){
		alignas(64) u8 storage[512];
		Arena a = arena_from_buffer(storage, sizeof(storage));

		struct Point { i32 x, y; };
		Point* points = (Point*)a.alloc(sizeof(Point) * 8, alignof(Point));
		check(points != nullptr && (uintptr)points % alignof(Point) == 0);
		bool zeroed = true;
		for(usize i = 0; i < 8; i++){ zeroed = zeroed && points[i].x == 0 && points[i].y == 0; }
		check(zeroed);

		Point* point = new (New_Tag{}, points) Point{12, 34};
		check(point == points && point->x == 12 && point->y == 34);

		Point* made = a.make<Point>(Point{5, 6});
		check(made != nullptr && made->x == 5 && made->y == 6);

		Slice<i32> nums = a.make_slice<i32>(4, 7);
		check(nums.len() == 4 && nums[0] == 7 && nums[3] == 7);
	});

	test("arena: allocator interface", [](Test& t){
		alignas(64) u8 storage[513];
		Arena a = arena_from_buffer(storage + 1, 512);

		Allocator allocator = a.allocator();
		check(allocator.query() == (Mem_Alloc | Mem_Grow | Mem_Shrink | Mem_FreeAll));
		allocator.free_all();
		u8* p = (u8*)allocator.alloc({16, 1});
		check(p == storage + 1);
		bool zeroed = true;
		for(usize i = 0; i < 16; i++){ zeroed = zeroed && p[i] == 0; p[i] = 42; }
		check(zeroed);

		// A stricter alignment must move even the most recent allocation.
		u8* moved = (u8*)allocator.grow(p, {16, 1}, {32, 64});
		check(moved && moved != p && (uintptr)moved % 64 == 0);
		bool kept = true;
		for(usize i = 0; i < 32; i++){ kept = kept && moved[i] == (i < 16 ? 42 : 0); }
		check(kept);

		check(allocator.grow(moved, {32, 64}, {48, 64}) == moved);
		zeroed = true;
		for(usize i = 32; i < 48; i++){ zeroed = zeroed && moved[i] == 0; }
		check(zeroed);

		check(allocator.shrink(moved, {48, 64}, {16, 64}) == moved);
		check(a.last_allocation_size == 16);

		usize offset = a.offset;
		allocator.free(moved, {16, 64});
		check(a.offset == offset && moved[0] == 42);
		check(allocator.grow(moved, {16, 64}, {SIZE_MAX, 64}) == nullptr);
		check(a.offset == offset && moved[0] == 42);

		check(allocator.alloc({1, 1}));
		p = (u8*)allocator.grow(moved, {16, 64}, {32, 64});
		check(p && p != moved);
		kept = true;
		for(usize i = 0; i < 32; i++){ kept = kept && p[i] == (i < 16 ? 42 : 0); }
		check(kept);

		allocator.free_all();
		check(a.offset == 0 && a.last_allocation == nullptr && a.last_allocation_size == 0);
		check(allocator.alloc({512, 1}) == storage + 1);
		check(allocator.alloc({1, 1}) == nullptr);
	});

	test("arena: empty", [](Test& t){
		Arena empty = arena_from_buffer(nullptr, 0);
		check(empty.alloc(1, 1) == nullptr && !empty.owns(nullptr));
		check(!empty.resize(nullptr, 1));
		empty.reset();
		check(empty.offset == 0);
	});
}

}
