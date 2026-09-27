#include <assert.h>
#include <type_traits>
#include <string.h>
#include "../base.hpp"

static_assert(std::is_aggregate_v<Arena>);
static_assert(std::is_trivial_v<Arena>);

int main(){
	alignas(64) u8 storage[513];
	memset(storage, 0xff, sizeof(storage));
	Arena a = arena_from_buffer(storage + 1, 512);
	assert(a.owns(storage + 1));
	assert(!a.owns(storage));
	assert(!a.owns(storage + 513));
	assert(!a.owns(nullptr));
	assert(a.alloc(0, 1) == nullptr);
	assert(a.alloc(1, 1) == storage + 1);
	u8* p = (u8*)a.alloc(32, 64);
	assert(p != nullptr && (uintptr)p % 64 == 0);
	for(usize i = 0; i < 32; i++){ assert(p[i] == 0); p[i] = 42; }
	assert(a.resize(p, 64));
	for(usize i = 0; i < 64; i++){ assert(p[i] == (i < 32 ? 42 : 0)); }
	assert(a.resize(p, 16));
	assert(a.resize(p, 32));
	for(usize i = 16; i < 32; i++){ assert(p[i] == 0); }
	usize offset = a.offset;
	assert(!a.resize(p, SIZE_MAX));
	assert(a.offset == offset && a.last_allocation_size == 32);
	assert(a.alloc(SIZE_MAX, 1) == nullptr);
	assert(a.offset == offset && a.last_allocation == p);

	assert(a.alloc(1, 1) != nullptr);
	assert(!a.resize(p, 64));
	u8* moved = (u8*)a.realloc(p, 32, 64, 64);
	assert(moved != nullptr && moved != p && (uintptr)moved % 64 == 0);
	for(usize i = 0; i < 64; i++){ assert(moved[i] == (i < 16 ? 42 : 0)); }
	assert(a.realloc(moved, 64, 80, 64) == moved);
	for(usize i = 64; i < 80; i++){ assert(moved[i] == 0); }
	offset = a.offset;
	assert(a.realloc(p, 32, SIZE_MAX, 64) == nullptr);
	assert(a.offset == offset && p[0] == 42);

	a.reset();
	assert(a.offset == 0 && a.last_allocation == nullptr && a.last_allocation_size == 0);
	assert(!a.resize(p, 8));
	assert(a.alloc(512, 1) == storage + 1);
	assert(a.alloc(1, 1) == nullptr);
	for(usize i = 1; i < sizeof(storage); i++){ assert(storage[i] == 0); }
	assert(storage[0] == 0xff);
	a.reset();
	assert(a.realloc(nullptr, 0, 16, 1) == storage + 1);
	assert(a.resize(storage + 1, 0));
	assert(a.offset == 0);
	assert(a.alloc(512, 1) == storage + 1);
	a.reset();
	struct Point { i32 x, y; };
	Point* points = (Point*)a.alloc(sizeof(Point) * 8, alignof(Point));
	assert(points != nullptr && (uintptr)points % alignof(Point) == 0);
	for(usize i = 0; i < 8; i++){ assert(points[i].x == 0 && points[i].y == 0); }
	Point* point = new (New_Tag{}, points) Point{12, 34};
	assert(point == points && point->x == 12 && point->y == 34);

	Allocator allocator = a.allocator();
	assert(allocator.query() == (Mem_Alloc | Mem_Grow | Mem_Shrink | Mem_FreeAll));
	allocator.free_all();
	p = (u8*)allocator.alloc({16, 1});
	assert(p == storage + 1);
	for(usize i = 0; i < 16; i++){ assert(p[i] == 0); p[i] = 42; }
	// A stricter alignment must move even the most recent allocation.
	moved = (u8*)allocator.grow(p, {16, 1}, {32, 64});
	assert(moved && moved != p && (uintptr)moved % 64 == 0);
	for(usize i = 0; i < 32; i++){ assert(moved[i] == (i < 16 ? 42 : 0)); }
	assert(allocator.grow(moved, {32, 64}, {48, 64}) == moved);
	for(usize i = 32; i < 48; i++){ assert(moved[i] == 0); }
	assert(allocator.shrink(moved, {48, 64}, {16, 64}) == moved);
	assert(a.last_allocation_size == 16);
	offset = a.offset;
	allocator.free(moved, {16, 64});
	assert(a.offset == offset && moved[0] == 42);
	assert(allocator.grow(moved, {16, 64}, {SIZE_MAX, 64}) == nullptr);
	assert(a.offset == offset && moved[0] == 42);
	assert(allocator.alloc({1, 1}));
	p = (u8*)allocator.grow(moved, {16, 64}, {32, 64});
	assert(p && p != moved);
	for(usize i = 0; i < 32; i++){ assert(p[i] == (i < 16 ? 42 : 0)); }
	allocator.free_all();
	assert(a.offset == 0 && a.last_allocation == nullptr && a.last_allocation_size == 0);
	assert(allocator.alloc({512, 1}) == storage + 1);
	assert(allocator.alloc({1, 1}) == nullptr);

	Arena empty = arena_from_buffer(nullptr, 0);
	assert(empty.alloc(1, 1) == nullptr && !empty.owns(nullptr));
	assert(!empty.resize(nullptr, 1));
	empty.reset();
}

#include "../base.cpp"
