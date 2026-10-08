#include "testing.hpp"
#include "../base.hpp"

namespace test {

static Heap_Allocator make_map_heap(){
	alignas(64) static u8 heap_buffer[1024 * 1024];
	mem_set(heap_buffer, 0xab, sizeof(heap_buffer));
	return heap_from_buffer({heap_buffer, sizeof(heap_buffer)});
}

// Every key lands on the same home slot
inline constexpr auto colliding_hash = [](i32 const&){ return u64(0); };

void map_tests(){
	test("map: insert, get, contains", [](Test& t){
		Heap_Allocator heap = make_map_heap();
		auto m = make_map<i32, i32>(0, heap.allocator());
		check(m.length == 0 && m.capacity == 0);
		check(!m.contains_key(1));
		check(!m.get(1).b);

		m.insert(1, 100);
		m.insert(2, 200);
		m.insert(-7, 700);
		check(m.length == 3);
		check(m.contains_key(1) && m.contains_key(2) && m.contains_key(-7));
		check(!m.contains_key(3));

		auto [v, found] = m.get(2);
		check(found && v == 200);
		check(m.get(-7).a == 700);

		// Overwrite keeps the length
		m.insert(2, 222);
		check(m.length == 3 && m.get(2).a == 222);

		m.destroy();
	});

	test("map: growth", [](Test& t){
		Heap_Allocator heap = make_map_heap();
		auto m = make_map<i32, i32>(8, heap.allocator());
		check(m.capacity == 8);

		for(i32 i = 0; i < 2000; i++){ m.insert(i * 7, i); }
		check(m.length == 2000);
		check(m.capacity >= 2000 && (m.capacity & (m.capacity - 1)) == 0);

		bool all_found = true;
		for(i32 i = 0; i < 2000; i++){
			auto [v, found] = m.get(i * 7);
			all_found = all_found && found && v == i;
		}
		check(all_found);
		check(!m.contains_key(3));

		m.destroy();
	});

	test("map: remove", [](Test& t){
		Heap_Allocator heap = make_map_heap();
		auto m = make_map<i32, i32>(0, heap.allocator());
		for(i32 i = 0; i < 500; i++){ m.insert(i, -i); }

		// Remove the even keys, odd ones must survive the backward shifting
		for(i32 i = 0; i < 500; i += 2){ m.remove(i); }
		check(m.length == 250);

		bool ok = true;
		for(i32 i = 0; i < 500; i++){
			auto [v, found] = m.get(i);
			ok = ok && (i % 2 == 0 ? !found : (found && v == -i));
		}
		check(ok);

		m.remove(12345);
		check(m.length == 250);

		// Reinsert after removal
		m.insert(4, 44);
		check(m.length == 251 && m.get(4).a == 44);

		m.destroy();
	});

	test("map: colliding hashes", [](Test& t){
		Heap_Allocator heap = make_map_heap();
		Map<i32, i32, colliding_hash> m = {
			.hashes = nullptr, .keys = nullptr, .vals = nullptr,
			.length = 0, .capacity = 0, .allocator = heap.allocator(),
		};

		for(i32 i = 0; i < 50; i++){ m.insert(i, i * 2); }
		check(m.length == 50);

		bool ok = true;
		for(i32 i = 0; i < 50; i++){ ok = ok && m.get(i).b && m.get(i).a == i * 2; }
		check(ok);

		// Removing from the middle of one long cluster
		m.remove(0);
		m.remove(25);
		m.remove(49);
		ok = m.length == 47;
		for(i32 i = 0; i < 50; i++){
			bool removed = i == 0 || i == 25 || i == 49;
			ok = ok && (removed ? !m.contains_key(i) : m.get(i).a == i * 2);
		}
		check(ok);

		m.destroy();
	});

	test("map: string keys", [](Test& t){
		Heap_Allocator heap = make_map_heap();
		auto m = make_map<String, i32>(0, heap.allocator());

		m.insert("apple", 1);
		m.insert("banana", 2);
		m.insert("cherry", 3);

		// Same contents through a different pointer must match
		char const buf[] = "banana split";
		check(m.get(String(buf, 6)).a == 2);
		check(m.contains_key("apple") && !m.contains_key("apples"));

		m.remove("apple");
		check(!m.contains_key("apple") && m.length == 2);

		m.destroy();
	});

	test("map: reset", [](Test& t){
		Heap_Allocator heap = make_map_heap();
		auto m = make_map<i32, i32>(0, heap.allocator());
		for(i32 i = 0; i < 20; i++){ m.insert(i, i); }

		m.reset();
		check(m.length == 0 && m.capacity == 0 && m.hashes == nullptr);
		check(!m.contains_key(3));

		m.insert(3, 33);
		check(m.get(3).a == 33);
		m.destroy();
	});
}

}
