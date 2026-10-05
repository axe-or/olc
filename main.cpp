#include "base.hpp"
#include "lib/tlsf.h"
#include <cerrno>

extern "C" int printf(char const*, ...);

template<typename T>
concept Eq = requires(T const& a, T const& b){
	{ a == b } -> Convertible_To<bool>;
};

template<typename H, typename T>
concept Hasher = requires(H h, T const& k){
	{ h(k) } -> Convertible_To<u64>;
};

template<typename T>
concept Unsigned_Integer = Same_As<T, unsigned char>
	|| Same_As<T, unsigned short>
	|| Same_As<T, unsigned int>
	|| Same_As<T, unsigned long>
	|| Same_As<T, unsigned long long>
	|| Same_As<T, u8>
	|| Same_As<T, u16>
	|| Same_As<T, u32>
	|| Same_As<T, u64>
;

template<typename T>
concept Signed_Integer = Same_As<T, char>
	|| Same_As<T, short>
	|| Same_As<T, int>
	|| Same_As<T, long>
	|| Same_As<T, long long>
	|| Same_As<T, i8>
	|| Same_As<T, i16>
	|| Same_As<T, i32>
	|| Same_As<T, i64>
;

template<typename T>
concept Integer = Signed_Integer<T> || Unsigned_Integer<T>;

template<typename T>
concept Float = Same_As<T, float> || Same_As<T, double>;

// Default hasher overload
template<typename T>
struct hash;

template<Integer T>
struct hash<T>{
	u64 operator()(T k){
		return u64(k);
	}
};

template<>
struct hash<double> {
	u64 operator()(double k) const {
		u64 h = bit_cast<u64>(k);
		h ^= h >> 30;
		h *= UINT64_C(0xbf58476d1ce4e5b9);
		h ^= h >> 27;
		h *= UINT64_C(0x94d049bb133111eb);
		h ^= h >> 31;
		return h;
	}
};

template<>
struct hash<float>{
	u64 operator()(float k) const {
		return hash<double>{}(k);
	}
};


template<>
struct hash<String>{
	u64 operator()(String const& k) const {
		return k.hash();
	}
};

template<Eq K, typename V, Hasher<K> auto hash = ::hash<K>{}>
struct Map {
	u64* hashes; // Important: hash == 0 indicates vacant slot. This is enforced locally with `hash_of`
	K*   keys;
	V*   vals;

	usize length;
	usize capacity;
	Allocator allocator;

	u64 hash_of(K const& key){
		u64 h = hash(key);
		return h == 0 ? 1 : h;
	}

	Pair<usize, bool> find_with_hash(K const& key, u64 h){
		if(capacity == 0){ return {0, false}; }
		usize mask = capacity - 1;
		usize home = h & mask;

		for(usize probe = 0; probe < length; probe += 1){
			usize pos = (home + probe) & mask;
			if(hashes[pos] == 0){
				break;
			}

			usize dist_pos = dist_from_home(pos);
			if(dist_pos < probe){
				break;
			}

			if(keys[pos] == key){
				return {pos, true};
			}
		}

		return {0, false};
	}

	Pair<usize, bool> find(K const& key){
		u64 h = hash_of(key);
		return find_with_hash(key, h);
	}

	usize dist_from_home(usize pos){
		usize mask = capacity - 1;
		return (pos - (hashes[pos] & mask)) & mask;
	}

	// Write the provided values into `pos`, writing back to parameters the previous values. This is
	// used to steal from the rich.
	void swap_with_slot(usize pos, u64* h, K* key, V* val){
		swap_ptr(&hashes[pos], h);
		swap_ptr(&keys[pos], key);
		swap_ptr(&vals[pos], val);
	}

	// Required Layout a single block holding hashes, keys and vals (in that order)
	static constexpr
	Memory_Layout block_layout(usize cap){
		constexpr usize align = max(max(alignof(u64), alignof(K)), alignof(V));
		// Hashes sit at the (max aligned) start, keys and vals may need up to align-1 bytes of padding each
		usize size = cap * sizeof(u64)
			+ cap * sizeof(K) + alignof(K) - 1
			+ cap * sizeof(V) + alignof(V) - 1;
		return {size, align};
	}

	// Grow to at least `new_cap` slots (rounded up to a power of 2), rehashing all entries
	void reserve(usize new_cap){
		if(new_cap <= capacity){ return; }

		usize cap = 1;
		while(cap < new_cap){ cap <<= 1; }

		Memory_Layout layout = block_layout(cap);
		void* block = allocator.alloc(layout);
		ensure(block != nullptr, "failed to allocate map storage");

		// Throwaway arena just to carve the block with correct alignment
		// IMPORTANT: The allocation order MUST be EXACTLY this one.
		Arena arena = arena_from_buffer(block, layout.size);
		u64* new_hashes = (u64*)arena.alloc(cap * sizeof(u64), alignof(u64));
		K*   new_keys   = (K*)arena.alloc(cap * sizeof(K), alignof(K));
		V*   new_vals   = (V*)arena.alloc(cap * sizeof(V), alignof(V));

		ensure(new_hashes == block && new_keys != nullptr && new_vals != nullptr, "bad map block layout");

		u64*  old_hashes = hashes;
		K*    old_keys   = keys;
		V*    old_vals   = vals;
		usize old_cap    = capacity;

		hashes   = new_hashes;
		keys     = new_keys;
		vals     = new_vals;
		capacity = cap;
		length   = 0; // insert_with_hash() counts entries back up

		for(usize i = 0; i < old_cap; i += 1){
			if(old_hashes[i] != 0){
				insert_with_hash(old_hashes[i], old_keys[i], old_vals[i]);
			}
		}

		// Pointer to hashes coincides with start of block, as asserted previously
		if(old_hashes != nullptr){
			allocator.free(old_hashes, block_layout(old_cap));
		}
	}

	void insert(K const& key, V const& val){
		u64 h = hash_of(key);
		insert_with_hash(h, key, val);
	}

	void insert_with_hash(u64 h, K key, V val){
		if(length + 1 > (capacity * 7) / 8){
			reserve(max(capacity * 2, usize(8)));
		}

		usize mask = capacity - 1;
		usize home = h & mask;

		// Distance of the current carry if it were to be placed in the current slot
		usize carry_dist = 0;

		for(usize probe = 0; probe < capacity; probe += 1){
			usize pos = (home + probe) & mask;

			if(hashes[pos] == 0){
				keys[pos] = key;
				vals[pos] = val;
				hashes[pos] = h;
				length += 1;
				return;
			}

			if(hashes[pos] == h && keys[pos] == key){
				vals[pos] = val;
				return;
			}

			usize dist_pos = dist_from_home(pos);
			if(dist_pos < carry_dist){
				swap_with_slot(pos, &h, &key, &val); // Steal richer slot and carry it forward
				carry_dist = dist_pos;
			}

			carry_dist += 1;
		}

		panic("map should not be full");
	}

	void remove(K key){
		u64 h = hash_of(key);
		remove_with_hash(key, h);
	}

	void remove_with_hash(K const& key, u64 h){
		if(length == 0){ return; }

		auto [removed_position, found] = find_with_hash(key, h);
		if(!found){ return; }

		usize mask = capacity - 1;
		usize hole_pos = removed_position;

		for(;;){
			usize next_position = (hole_pos + 1) & mask;

			bool next_is_empty   = hashes[next_position] == 0;
			bool next_is_at_home = !next_is_empty && dist_from_home(next_position) == 0;
			if(next_is_empty || next_is_at_home){
				break;
			}

			hashes[hole_pos] = hashes[next_position];
			keys[hole_pos]   = keys[next_position];
			vals[hole_pos]   = vals[next_position];

			hole_pos = next_position;
		}

		// Only the hash marks occupancy; stale key/val bytes are fine for trivial types
		hashes[hole_pos] = 0;
		length -= 1;
	}

	// Completely reset, freeing all memory
	auto reset(){
		allocator.free(hashes, block_layout(capacity));

		capacity = 0;
		length = 0;
		hashes = nullptr;
		keys = nullptr;
		vals = nullptr;

		return this;
	}

	void destroy(){
		reset();
	}
};

template<Eq K, typename V>
auto make_map(usize capacity, Allocator a){
	auto m = Map<K, V>{
		.hashes = nullptr,
		.keys = nullptr,
		.vals = nullptr,
		.length = 0,
		.capacity = 0,
		.allocator = a,
	};

	m.reserve(capacity);
	return m;
}

void entrypoint(){
}

struct Heap_Allocator {
	tlsf_t impl;

	void* alloc(Memory_Layout layout) {
		return tlsf_memalign(impl, layout.align, layout.size);
	}

	void free(void* p) {
		return tlsf_free(impl, p);
	}

	void* realloc(void* ptr, Memory_Layout old, Memory_Layout desired) {
		if(old.align == desired.align){
			return tlsf_realloc(impl, ptr, desired.size);
		}

		void* data = tlsf_memalign(impl, desired.align, desired.size);
		usize n = min(old.size, desired.size);
		if(data){
			mem_copy_no_overlap(data, ptr, n);
			tlsf_free(impl, ptr);
			if(old.size < desired.size){
				void* dest = (void*)(uintptr(data) + old.size);
				mem_zero(dest, desired.size - old.size);
			}
		}
		return data;
	}

	Allocator allocator();
};

Heap_Allocator heap_from_buffer(Slice<u8> buf){
	ensure(tlsf_size() > buf.len(), "not enough space for tlsf metadata");
	auto impl = tlsf_create_with_pool(buf.raw_data(), buf.len());
	ensure(impl != NULL, "failed to create with pool");
	return Heap_Allocator{ impl };
}

static
uintptr heap_allocator_proc(void* impl, Allocator_Mode mode, void* ptr, Memory_Layout old, Memory_Layout desired) {
	auto heap = (Heap_Allocator*)impl;
	void* res = nullptr;

	switch (mode) {
		case Mem_Query:
			return Mem_Alloc | Mem_Grow | Mem_Shrink | Mem_Free;

		case Mem_Alloc:
			res = heap->alloc(desired);
			return (uintptr)res;

		case Mem_Grow:
				res = heap->realloc(ptr, old, desired);
			return (uintptr)res;

			case Mem_Shrink: {
				return (uintptr)heap->realloc(ptr, old, desired);
			}

		case Mem_Free:
			heap->free(ptr);
			return 0;

		case Mem_FreeAll:
			return 0; /* Unsupported */
	}

	panic("invalid allocator mode");
}

Allocator Heap_Allocator::allocator(){
	return {
		.impl_ = this,
		.proc_ = heap_allocator_proc,
	};
}

void init(){
	u8 static allocator_data[16ull * 1024ull * 1024ull] = {0};
	auto heap = heap_from_buffer({&allocator_data[0], sizeof(allocator_data)});
}

int main(){
	init();
	entrypoint();
}

#include "base.cpp"
