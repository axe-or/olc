#include "testing.hpp"
#include "../base.hpp"

namespace test {

static bool dyn_array_equals(Dyn_Array<i32> const& arr, Slice<i32> expected){
	if(arr.length != expected.len()){ return false; }
	for(usize i = 0; i < arr.length; i++){
		if(arr[i] != expected[i]){ return false; }
	}
	return true;
}

#define EXPECT_ITEMS(arr, ...) [&]{ i32 items_[] = {__VA_ARGS__}; return dyn_array_equals((arr), Slice<i32>{items_, sizeof(items_) / sizeof(i32)}); }()

bool dyn_array_tests(){
	bool ok = true;
	ok &= test("dyn_array: append and pop", [](Test& t){
		alignas(16) static u8 storage[64 * 1024];
		Arena arena = arena_from_buffer(storage, sizeof(storage));
		auto arr = make_dynamic_array<i32>(4, arena.allocator());
		check(arr.length == 0 && arr.capacity == 4 && arr.data != nullptr);

		for(i32 i = 0; i < 100; i++){ arr.append(i); }
		check(arr.length == 100 && arr.capacity >= 100);
		bool in_order = true;
		for(i32 i = 0; i < 100; i++){ in_order = in_order && arr[usize(i)] == i; }
		check(in_order);

		check(arr.pop());
		check(arr.length == 99 && arr[98] == 98);
		while(arr.pop()){}
		check(arr.length == 0);
		check(!arr.pop());
	});

	ok &= test("dyn_array: insert and remove", [](Test& t){
		alignas(16) static u8 storage[64 * 1024];
		Arena arena = arena_from_buffer(storage, sizeof(storage));
		auto arr = make_dynamic_array<i32>(0, arena.allocator());

		check(arr.insert(0, 2));
		check(arr.insert(0, 0));
		check(arr.insert(1, 1));
		check(arr.insert(3, 3));
		check(EXPECT_ITEMS(arr, 0, 1, 2, 3));
		check(!arr.insert(5, 9));

		check(arr.remove(1));
		check(EXPECT_ITEMS(arr, 0, 2, 3));
		check(arr.remove(2));
		check(EXPECT_ITEMS(arr, 0, 2));
		check(!arr.remove(2));
	});

	ok &= test("dyn_array: swap insert and remove", [](Test& t){
		alignas(16) static u8 storage[64 * 1024];
		Arena arena = arena_from_buffer(storage, sizeof(storage));
		auto arr = make_dynamic_array<i32>(0, arena.allocator());

		for(i32 i = 0; i < 4; i++){ arr.append(i * 10); }
		check(arr.insert_swap(1, 99));
		check(EXPECT_ITEMS(arr, 0, 99, 20, 30, 10));
		check(!arr.insert_swap(6, 1));

		check(arr.remove_swap(0));
		check(EXPECT_ITEMS(arr, 10, 99, 20, 30));
		check(arr.remove_swap(3));
		check(EXPECT_ITEMS(arr, 10, 99, 20));
		check(!arr.remove_swap(3));
	});

	ok &= test("dyn_array: views, reserve, shrink", [](Test& t){
		alignas(16) static u8 storage[64 * 1024];
		Arena arena = arena_from_buffer(storage, sizeof(storage));
		auto arr = make_dynamic_array<i32>(2, arena.allocator());
		for(i32 i = 0; i < 6; i++){ arr.append(i); }

		check(arr.take(2).len() == 2 && arr.take(2)[1] == 1);
		check(arr.skip(4).len() == 2 && arr.skip(4)[0] == 4);
		check(arr.slice(1, 3).len() == 2 && arr.slice(1, 3)[1] == 2);

		check(arr.reserve(64));
		check(arr.capacity == 64 && EXPECT_ITEMS(arr, 0, 1, 2, 3, 4, 5));
		check(arr.reserve(8) && arr.capacity == 64);

		check(arr.shrink_to_fit());
		check(arr.capacity == 6 && EXPECT_ITEMS(arr, 0, 1, 2, 3, 4, 5));

		arr.destroy();
	});

	ok &= test("dyn_array: heap backed", [](Test& t){
		alignas(16) static u8 storage[256 * 1024];
		Heap_Allocator heap = heap_from_buffer({storage, sizeof(storage)});
		auto arr = make_dynamic_array<u64>(0, heap.allocator());
		for(u64 i = 0; i < 1000; i++){ arr.append(i * i); }
		check(arr.length == 1000 && arr[999] == 999 * 999 && arr[10] == 100);
		arr.destroy();
	});
	return ok;
}

#undef EXPECT_ITEMS

}
