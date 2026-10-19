#include "testing.hpp"
#include "../base.hpp"

static_assert(__is_aggregate(Typed_Arena_ID<i32>));
static_assert(sizeof(Typed_Arena_ID<i32>) == sizeof(u32));
static_assert(sizeof(Typed_Arena_ID<i32, u16>) == sizeof(u16));

namespace test {

bool typed_arena_tests(){
	bool ok = true;
	ok &= test("typed_arena: put and get", [](Test& t){
		alignas(16) static u8 storage[64 * 1024];
		Arena arena = arena_from_buffer({storage, sizeof(storage)});
		auto ta = make_typed_arena<i32>(4, arena.allocator());
		check(ta.storage.len() == 0 && ta.storage.cap() == 4);

		auto a = ta.put(10);
		auto b = ta.put(20);
		auto c = ta.put(30);
		check(a.v != 0 && b.v != 0 && c.v != 0);
		check(a != b && b != c && a != c);
		check(ta.storage.len() == 3);

		check(ta.get(a) && *ta.get(a) == 10);
		check(ta.get(b) && *ta.get(b) == 20);
		check(ta.get(c) && *ta.get(c) == 30);
		check(ta[a] == 10 && ta[b] == 20 && ta[c] == 30);

		ta[b] = 22;
		check(*ta.get(b) == 22);
		*ta.get(c) = 33;
		check(ta[c] == 33);

		auto const& cta = ta;
		check(cta.get(a) && *cta.get(a) == 10);
		check(cta[b] == 22 && cta[c] == 33);
	});

	ok &= test("typed_arena: invalid IDs", [](Test& t){
		alignas(16) static u8 storage[64 * 1024];
		Arena arena = arena_from_buffer({storage, sizeof(storage)});
		auto ta = make_typed_arena<i32>(0, arena.allocator());
		using ID = decltype(ta)::ID;

		// Zero is never a valid ID, even on an empty arena
		check(!ID{0});
		check(ta.get(ID{0}) == nullptr);
		check(ta.get(ID{1}) == nullptr);

		auto a = ta.put(1);
		check(!!a);
		check(ta.get(ID{0}) == nullptr);
		check(ta.get(ID{a.v + 1}) == nullptr);
		check(ta.get(ID{~u32(0)}) == nullptr);

		auto const& cta = ta;
		check(cta.get(ID{0}) == nullptr && cta.get(ID{a.v + 1}) == nullptr);
	});

	ok &= test("typed_arena: growth", [](Test& t){
		alignas(16) static u8 storage[256 * 1024];
		Heap_Allocator heap = heap_from_buffer({storage, sizeof(storage)});
		auto ta = make_typed_arena<u64>(0, heap.allocator());
		using ID = decltype(ta)::ID;

		ID ids[1000];
		for(u64 i = 0; i < 1000; i++){ ids[i] = ta.put(i * i); }
		check(ta.storage.len() == 1000);

		// IDs stay valid across reallocations and are handed out in order
		bool matches = true;
		for(u64 i = 0; i < 1000; i++){
			matches = matches && ta.get(ids[i]) && *ta.get(ids[i]) == i * i;
			if(i > 0){ matches = matches && ids[i - 1] < ids[i]; }
		}
		check(matches);

		ta.destroy();
	});

	ok &= test("typed_arena: put failure", [](Test& t){
		alignas(16) u8 storage[2 * sizeof(i64)];
		Arena arena = arena_from_buffer({storage, sizeof(storage)});
		auto ta = make_typed_arena<i64>(2, arena.allocator());

		check(!!ta.put(1));
		check(!!ta.put(2));
		// Backing arena is full, so growing must fail and hand out the null ID
		auto bad = ta.put(3);
		check(!bad);
		check(ta.storage.len() == 2);
	});

	ok &= test("typed_arena: reset", [](Test& t){
		alignas(16) static u8 storage[64 * 1024];
		Arena arena = arena_from_buffer({storage, sizeof(storage)});
		auto ta = make_typed_arena<i32>(4, arena.allocator());

		auto a = ta.put(1);
		ta.put(2);
		ta.reset();
		check(ta.storage.len() == 0);
		check(ta.get(a) == nullptr);

		// IDs get reused after a reset
		auto b = ta.put(7);
		check(a == b && ta[b] == 7);
	});

	ok &= test("typed_arena: small ID type", [](Test& t){
		alignas(16) static u8 storage[64 * 1024];
		Arena arena = arena_from_buffer({storage, sizeof(storage)});
		auto ta = make_typed_arena<i32, u16>(0, arena.allocator());

		auto a = ta.put(5);
		auto b = ta.put(6);
		check(a < b && ta[a] == 5 && ta[b] == 6);
	});

	ok &= test("typed_arena: ID overflow", [](Test& t){
		alignas(16) static u8 storage[64 * 1024];
		Arena arena = arena_from_buffer({storage, sizeof(storage)});
		auto ta = make_typed_arena<i32, u8>(0, arena.allocator());

		bool all_put = true;
		for(i32 i = 0; i < max_val<u8>; i++){ all_put = all_put && ta.put(i).v == u8(i + 1); }
		check(all_put);
		check(ta.storage.len() == max_val<u8>);

		// Out of IDs, must fail without storing anything
		auto bad = ta.put(-1);
		check(!bad);
		check(ta.storage.len() == max_val<u8>);
		check(ta[{max_val<u8>}] == max_val<u8> - 1);

		ta.reset();
		check(!!ta.put(7));
	});

	ok &= test("typed_arena: struct elements", [](Test& t){
		alignas(16) static u8 storage[64 * 1024];
		Arena arena = arena_from_buffer({storage, sizeof(storage)});

		struct Node { String name; i32 value; };
		auto ta = make_typed_arena<Node>(0, arena.allocator());

		auto a = ta.put(Node{"alpha", 1});
		auto b = ta.put(Node{"beta", 2});
		check(ta[a].name == "alpha" && ta[a].value == 1);
		check(ta.get(b)->name == "beta" && ta.get(b)->value == 2);

		ta[a].value = 100;
		check(ta.get(a)->value == 100);
	});
	return ok;
}

}
