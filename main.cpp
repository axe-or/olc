#include <stdio.h>
#include <assert.h>
#include "base.hpp"

template<typename T>
concept Eq = requires(T const& a, T const& b){
	{ a == b } -> Convertible_To<bool>;
};

template<typename T>
concept Hash = Eq<T> && (requires(T const& obj){ { obj.hash() } -> Convertible_To<u64>; } || Convertible_To<T const&, u64>);

template<Hash K, typename V>
struct Map {
	u64* hashes; // Important: hash == 0 indicates vacant slot
	K*   keys;
	V*   vals;

	usize length;
	usize capacity;
	Allocator allocator;

	static
	u64 hash_of(K const& key){
		u64 h = key.hash();
		return h == 0 ? 1 : h;
	}

	u64 desired_position(K const& key) {
		u64 h = hash_of(key);
		return usize(h) & (capacity - 1);
	}

	Pair<usize, bool> find(K const& key){
		if(capacity == 0){ return {0, false}; }

		usize mask = capacity - 1;
		usize home = desired_position(key);

		for(usize probe = 0; probe < length; probe += 1){
			usize pos = (home + probe) & mask;
			if(hashes[pos] == 0){
				break;
			}

			// How far the entry at pos is distant from its home
			usize dist_pos = (pos - (hashes[pos] & mask)) & mask;
			if(dist_pos < probe){
				break;
			}

			if(keys[pos] == key){
				return {pos, true};
			}
		}

		return {0, false};
	}

	void swap_with_slot(usize pos, u64* h, K* key, V* val){
		swap_ptr(&hashes[pos], h);
		swap_ptr(&keys[pos], key);
		swap_ptr(&vals[pos], val);
	}

	// required Layout a single block holding hashes, keys and vals (in that order)
	static
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

	void insert(K key, V val){
		if(length + 1 > (capacity * 7) / 8){
			reserve(max(capacity * 2, usize(8)));
		}
		u64 h = hash_of(key);
		insert_with_hash(h, key, val);
	}

	void insert_with_hash(u64 h, K key, V val){
		usize mask = capacity - 1;
		usize home = h & mask;

		// Distance of the current carry if it were to be placed in the current slot
		usize d = 0;

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

			// How far the entry at pos is distant from its home
			usize dist_pos = (pos - (hashes[pos] & mask)) & mask;
			if(dist_pos < d){
				swap_with_slot(pos, &h, &key, &val);
				d = dist_pos;
			}

			d += 1;
		}

		panic("map should not be full");
	}
};

int main(){
	static_assert(Hash<String>, "");
	static_assert(Hash<float>, "");
}

#include "base.cpp"
