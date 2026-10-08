#pragma once

//// Attributes and Compiler specifics
#if __STDC_VERSION__ >= 202311L
	/* Nice, we have native typeof support */
#else
	#if defined(__clang__) || defined(__GNUC__) || defined(_MSC_VER) || defined(__TINYC__)
		#define typeof __typeof__
	#else
		#error "Could not define typeof macro"
	#endif
#endif

#if defined(_MSC_VER)
	#define attribute_force_inline      __forceinline
#elif defined(__clang__) || defined(__GNUC__)
	#define attribute_force_inline      __attribute__((always_inline))
#else
	#define attribute_force_inline
	#define attribute_force_inline_func static inline
#endif

#if defined(__clang__) || defined(__GNUC__)
	#define attribute_format(fmt_pos, args_pos) __attribute__((format (printf, fmt_pos, args_pos)))
#else
	#define attribute_format(fmt, args)
#endif

//// Auto platform detection
#if !defined(BUILD_PLATFORM_WINDOWS) && !defined(BUILD_PLATFORM_LINUX) && !defined(BUILD_PLATFORM_WASI)
	#if defined(_WIN32) || defined(_WIN64)
		#define BUILD_PLATFORM_WINDOWS
	#elif defined(__linux__)
		#define BUILD_PLATFORM_LINUX
	#elif defined(__wasi__)
		#define BUILD_PLATFORM_WASI
	#endif
#endif

#if defined(BUILD_PLATFORM_WINDOWS)
	#define BUILD_HAS_VIRTUAL_MEMORY 1

	#ifndef _CRT_SECURE_NO_WARNINGS
		#define _CRT_SECURE_NO_WARNINGS
	#endif

	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
#elif defined(BUILD_PLATFORM_LINUX)
	#define BUILD_HAS_VIRTUAL_MEMORY 1

	#ifndef _DEFAULT_SOURCE
		#define _DEFAULT_SOURCE
	#endif
#elif defined(BUILD_PLATFORM_WASI)
	// WASI does not expose process virtual memory management APIs.
	#define BUILD_HAS_VIRTUAL_MEMORY 0
#else
	#error "Undefined platform macro"
#endif

#include <stddef.h>
#include <stdarg.h>
#include <stdalign.h>
#include <stdint.h>
#include <stdbool.h>
#include <atomic>
#include <bit>
#include <source_location>

using std::bit_cast;

//// Basic types & Utilities
using i8 = int8_t;
using u8 = uint8_t;

using i16 = int16_t;
using u16 = uint16_t;

using i32 = int32_t;
using u32 = uint32_t;

using i64 = int64_t;
using u64 = uint64_t;

using f32 = float;
using f64 = double;

using rune = int32_t;
using uintptr = uintptr_t;

using usize = size_t;
using isize = ptrdiff_t;

using cstring = const char*;

template<typename T>
using Atomic = std::atomic<T>;

using Source_Location = std::source_location;

#define caller_location std::source_location caller_loc = std::source_location::current()

#define min(x, y) (((x) < (y)) ? (x) : (y))

#define max(x, y) (((x) > (y)) ? (x) : (y))

#define clamp(lo, x, hi) min(max((lo), (x)), (hi))

template<typename T>
void swap_ptr(T* a, T* b){
	T tmp = *a;
	*a = *b;
	*b = tmp;
}

template<typename A, typename B = A>
struct Pair {
	A a;
	B b;
};

// Helpers that use preprocessor expansion tricks to "glue" identifiers
#define ident_concat0(x, y) x##y
#define ident_concat1(x, y) ident_concat0(x, y)
#define ident_concat2(x, y) ident_concat1(x, y)
#define ident_concat3(x, y) ident_concat2(x, y)
#define ident_concat4(x, y) ident_concat3(x, y)
#define ident_concat5(x, y) ident_concat4(x, y)
#define ident_concat6(x, y) ident_concat5(x, y)
#define ident_concat7(x, y) ident_concat6(x, y)

#define ident_concat(x, y)  ident_concat7(x, y)
#define ident_counter(x)    ident_concat(x, __COUNTER__)

//// Assertions

// Exit the program fatally
[[noreturn]] void trap();

// Exit the program fatally with a message
[[noreturn]] void panic(cstring msg, caller_location);

// Exit the program fatally with a message if a predicate fails
bool ensure(bool pred, cstring msg, caller_location);

// Same as `ensure` but only when BUILD_DEBUG is defined
#if defined(BUILD_DEBUG)
	#define ensure_debug(pred, msg) ensure((pred), (msg), std::source_location::current())
#else
	#define ensure_debug(pred, msg)
#endif

// Helper for unimplemented sections of code
[[noreturn]] static inline
void todo(caller_location){
	panic("TODO", caller_loc);
}

// Just a tag type for in-place new
struct New_Tag {};

inline void* operator new(usize, New_Tag, void* storage) noexcept {
	return storage;
}

//// Type traits

namespace detail {
	template<typename T>
	struct Remove_Reference { using Type = T; };

	template<typename T>
	struct Remove_Reference<T&> { using Type = T; };

	template<typename T>
	struct Remove_Reference<T&&> { using Type = T; };

	template<typename T>
	inline constexpr bool is_lvalue_reference = false;

	template<typename T>
	inline constexpr bool is_lvalue_reference<T&> = true;

	template<typename T, typename U>
	inline constexpr bool is_same = false;

	template<typename T>
	inline constexpr bool is_same<T, T> = true;

	template<typename T, typename U>
	concept same_as = is_same<T, U>;

	template<typename T>
	inline constexpr bool is_void = is_same<T, void>
		|| is_same<T, const void>
		|| is_same<T, volatile void>
		|| is_same<T, const volatile void>;

	// Unevaluated expressions only: int wins unless T&& is invalid (e.g. void).
	template<typename T>
	T&& declval(int) noexcept;

	template<typename T>
	T declval(...) noexcept;

	// Passing an argument checks implicit conversion to T.
	template<typename T>
	void accept_convert(T) noexcept;
}

template<typename T>
using Remove_Reference = typename detail::Remove_Reference<T>::Type;

template<typename T>
constexpr Remove_Reference<T>&& move(T&& value) noexcept {
	return static_cast<Remove_Reference<T>&&>(value);
}

template<typename T>
constexpr T&& forward(Remove_Reference<T>& value) noexcept {
	return static_cast<T&&>(value);
}

template<typename T>
constexpr T&& forward(Remove_Reference<T>&& value) noexcept {
	static_assert(!detail::is_lvalue_reference<T>, "cannot forward an rvalue as an lvalue");
	return static_cast<T&&>(value);
}

template<typename From, typename To>
concept Convertible_To = (detail::is_void<From> && detail::is_void<To>) || requires {
	// Functions cannot return arrays or functions so they must be rejected before
	// `accept_convert` parameter adjustment implicitly turns them into pointers.
	static_cast<To (*)()>(nullptr);
	detail::accept_convert<To>(detail::declval<From>(0));
	static_cast<To>(detail::declval<From>(0));
};

template<typename T, typename U>
concept Same_As = detail::same_as<T, U> && detail::same_as<U, T>;

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

// `F` can be called with `Args...` and its result converts to `R`
template<typename F, typename R, typename ... Args>
concept Callable = requires(F f, Args... args){
	{ f(args...) } -> Convertible_To<R>;
};

template<typename F, typename ... Args>
concept Predicate = Callable<F, bool, Args...>;

//// Slice
template<typename T>
struct Slice {
private:
	T* data_;
	usize len_;

public:
	T& operator[](usize idx){
		ensure(idx < len_, "slice index out of bounds");
		return data_[idx];
	}

	T const& operator[](usize idx) const{
		ensure(idx < len_, "slice index out of bounds");
		return data_[idx];
	}

	Slice<T> take(usize n){
		ensure(n <= len_, "cannot take more than length");
		return Slice<T>{ data_, n };
	}

	Slice<T> skip(usize n){
		ensure(n <= len_, "cannot take more than length");
		return Slice<T>{ &data_[n], len_ - n };
	}

	Slice<T> slice(usize start, usize end){
		ensure(start <= end && end <= len_, "invalid slice indices");
		return Slice<T>{ &data_[start], end - start };
	}

	constexpr Slice() : data_{nullptr}, len_{0}{}

	constexpr Slice(T* p, usize n) : data_{p}, len_{n}{}

	attribute_force_inline constexpr auto len(){ return len_; }
	attribute_force_inline constexpr auto raw_data(){ return data_; }
};

template<typename T>
Slice<T> copy(Slice<T> dst, Slice<T> src){
	usize n = min(dst.len(), src.len());

	T* d = dst.raw_data();
	T* s = src.raw_data();
	for(usize i = 0; i < n; i += 1){
		d[i] = s[i];
	}

	return Slice<T>{ d, n };
}

//// Hashing

// MurmurHash64A, seed zero, with little-endian byte order.
// Based on Austin Appleby's public-domain MurmurHash64A:
// https://github.com/aappleby/smhasher/blob/master/src/MurmurHash2.cpp
// Accept char directly: pointer reinterpretation is not allowed in constant expressions.
constexpr u64 hash_murmur64(Slice<u8> buf){
	constexpr u64 m = 0xc6a4a7935bd1e995ULL;
	constexpr u32 r = 47;
	u8 const* data = buf.raw_data();
	usize remaining = buf.len();
	u64 h = (u64)remaining * m;

	while(remaining >= 8){
		// Byte loads support unaligned input and fix the byte order.
		u64 k = 0;
		for(u32 i = 0; i < 8; i += 1){
			k |= (u64)(u8)data[i] << (8 * i);
		}
		k *= m;
		k ^= k >> r;
		k *= m;
		h ^= k;
		h *= m;
		data += 8;
		remaining -= 8;
	}

	for(usize i = 0; i < remaining; i += 1){
		h ^= (u64)(u8)data[i] << (8 * i);
	}
	if(remaining != 0){ h *= m; }
	h ^= h >> r;
	h *= m;
	h ^= h >> r;
	return h;
}

//// String

// Length of a C-style string
static inline constexpr
usize cstring_len(cstring cs) {
	usize n = 0;
	while(cs[n] != 0){
		n += 1;
	}
	return n;
}

// UTF-8 encoded slice of bytes
struct String {
private:
	char const* data_;
	usize len_;

public:
	u8 operator[](usize idx) const {
		ensure(idx < len_, "slice index out of bounds");
		return data_[idx];
	}

	String take(usize n){
		ensure(n <= len_, "cannot take more than length");
		return String{ data_, n };
	}

	String skip(usize n){
		ensure(n <= len_, "cannot take more than length");
		return String{ &data_[n], len_ - n };
	}

	String slice(usize start, usize end){
		ensure(start <= end && end <= len_, "invalid slice indices");
		return String{ &data_[start], end - start };
	}

	u64 hash() const {
		return hash_murmur64(Slice<u8>{ (u8*)data_, len_ });
	}

	constexpr String() : data_{nullptr}, len_{0}{}

	constexpr String(cstring cs) : data_{cs}, len_{cstring_len(cs)}{}

	constexpr String(char const* p, usize n) : data_{p}, len_{n}{}

	attribute_force_inline constexpr auto len(){ return len_; }
	attribute_force_inline constexpr auto raw_data(){ return data_; }

	constexpr bool operator==(String s) const {
		if(s.len_ != len_){ return false; }
		for(usize i = 0; i < len_; i ++){
			if(data_[i] != s.data_[i]){ return false; }
		}
		return true;
	}

	constexpr bool operator!=(String s) const {
		return !(*this == s);
	}
};

// Helper macro to use sized string with printf's "%.*s"
#define STRF(S) ((int)((S).len)), ((u8 const*)((S).v))

// The error unicode codepoint
constexpr rune RUNE_ERROR = 0xfffd;

// Decoded form of a unicode codepoint
struct Rune_Decoded {
	rune codepoint;
	i32  size;
};

// Encoded form of a unicode codepoint
struct Rune_Encoded {
	u8  bytes[4];
	i32 size;
};

// Encode a codepoint `r` to UTF-8
Rune_Encoded rune_encode(rune r);

// Decode the first rune of a UTF-8 encoded buffer
Rune_Decoded rune_decode(u8 const* buf, u32 buflen);

//// Memory

extern "C" {
	void* memmove(void*, void const*, size_t);
	void* memcpy(void*, void const*, size_t);
	void* memset(void*, int, size_t);
}

static inline attribute_force_inline
void* mem_copy(void* d, void const* s, usize n){
	return memmove(d, s, n);
}

static inline attribute_force_inline
void* mem_copy_no_overlap(void* d, void const* s, usize n){
	return memcpy(d, s, n);
}

static inline attribute_force_inline
void* mem_set(void* d, u8 v, usize n){
	return memset(d, v, n);
}

static inline attribute_force_inline
void* mem_zero(void* d, usize n){
	return memset(d, 0, n);
}

//// Allocator

enum Allocator_Mode : u8 {
	Mem_Query = 0, // Check which operations the allocator supports as a bitset of the other modes

	Mem_Alloc   = 1 << 0, // New allocation (zero-filled)
	Mem_Grow    = 1 << 1, // Grow allocation (excess is zero filled)
	Mem_Shrink  = 1 << 2, // Shrink allocation
	Mem_Free    = 1 << 3, // Free allocation
	Mem_FreeAll = 1 << 4, // Free all memory
};

struct Memory_Layout {
	usize size;
	usize align;
};

constexpr
bool valid_alignment(usize align){
	return align != 0 && (align & (align - 1)) == 0;
}

template<typename T>
constexpr auto layout_of(usize count = 0){
	return Memory_Layout{ sizeof(T) * count, alignof(T) };
}

using Allocator_Proc = uintptr (*) (void* impl, Allocator_Mode mode, void* ptr, Memory_Layout old, Memory_Layout desired);

struct Allocator {
	void* impl_;
	Allocator_Proc proc_;

	attribute_force_inline u8 query() const {
		return proc_(impl_, Mem_Query, nullptr, {}, {});
	}

	attribute_force_inline void* alloc(Memory_Layout desired) const {
		return (void*)(proc_(impl_, Mem_Alloc, nullptr, {}, desired));
	}

	attribute_force_inline void* grow(void* ptr, Memory_Layout old, Memory_Layout desired) const {
		ensure(desired.size >= old.size, "grow() must increase allocation size");
		return (void*)(proc_(impl_, Mem_Grow, ptr, old, desired));
	}

	attribute_force_inline void* shrink(void* ptr, Memory_Layout old, Memory_Layout desired) const {
		ensure(desired.size <= old.size, "shrink() must decrease allocation size");
		return (void*)(proc_(impl_, Mem_Shrink, ptr, old, desired));
	}

	attribute_force_inline void free(void* ptr, Memory_Layout old) const {
		proc_(impl_, Mem_Free, ptr, old, {});
	}

	attribute_force_inline void free_all() const {
		proc_(impl_, Mem_FreeAll, nullptr, {}, {});
	}
};

//// Arena
struct Arena {
	u8* data;
	usize capacity;
	usize offset;
	void* last_allocation;
	usize last_allocation_size;

	bool owns(void const* ptr) const;

	void* alloc(usize size, usize align);

	bool resize(void* ptr, usize new_size);

	void* realloc(void* ptr, usize old_size, usize new_size, usize align);

	void reset();

	Allocator allocator();

	template<typename T, typename ...Args>
	T* make(Args&& ...args){
		void* storage = alloc(sizeof(T), alignof(T));
		if(storage == nullptr){ return nullptr; }
		return new (New_Tag{}, storage) T(::forward<Args>(args)...);
	}

	template<typename T, typename ...Args>
	Slice<T> make_slice(usize count, Args&& ...args){
		T* p = static_cast<T*>(alloc(sizeof(T) * count, alignof(T)));
		if(p == nullptr){ return {}; }

		for(usize i = 0; i < count; i += 1){
			new (New_Tag{}, p + i) T(::forward<Args>(args)...);
		}
		return Slice<T>{ p, count };
	}
};

Arena arena_from_buffer(void* buffer, usize size);

//// Heap allocator
struct Heap_Allocator {
	void* impl;

	void* alloc(Memory_Layout layout);

	void free(void* p);

	void* realloc(void* ptr, Memory_Layout old, Memory_Layout desired);

	Allocator allocator();
};

Heap_Allocator heap_from_buffer(Slice<u8> buf);

//// Dynamic array

template<typename T>
struct Dyn_Array {
	T* data;
	usize length;
	usize capacity;

	Allocator allocator;

	bool reserve(usize desired){
		if(capacity >= desired){
			return true;
		}

		void* p = allocator.grow(data, layout_of<T>(capacity), layout_of<T>(desired));
		if(!p){
			return false;
		}

		data = (T*)p;
		capacity = desired;
		return true;
	}

	bool shrink_to_fit(){
		void* p = allocator.shrink(data, layout_of<T>(capacity), layout_of<T>(length));
		if(!p){ return false; }
		data = (T*)p;
		capacity = length;
		return true;
	}

	// Insert value at `idx`, shifting all elements
	bool insert(usize idx, T const& val){
		if(idx > length){ return false; }
		if(length == capacity){
			usize desired = max(usize(16), capacity * 2);
			if(desired < capacity || !reserve(desired)){ return false; }
		}
		for(usize i = length; i > idx; i -= 1){
			data[i] = data[i - 1];
		}
		data[idx] = val;
		length += 1;
		return true;
	}

	// Insert value at `idx` by swapping the value at position with the last value
	bool insert_swap(usize idx, T const& val){
		if(idx > length){ return false; }
		if(length == capacity){
			usize desired = capacity == 0 ? 8 : capacity * 2;
			if(desired < capacity || !reserve(desired)){
				return false;
			}
		}

		if(idx < length){
			data[length] = data[idx];
		}

		data[idx] = val;
		length += 1;
		return true;
	}

	// Remove value `idx`, shifting all elements
	bool remove(usize idx){
		if(idx >= length){ return false; }
		for(usize i = idx; i + 1 < length; i += 1){
			data[i] = data[i + 1];
		}
		length -= 1;
		return true;
	}

	// Remove value at `idx` by swapping the value at position with the last value
	bool remove_swap(usize idx){
		if(idx >= length){ return false; }
		length -= 1;
		if(idx < length){
			data[idx] = data[length];
		}
		return true;
	}

	// Push item to end of array
	bool append(T const& val){
		return insert_swap(length, val);
	}

	// Pop last item of array
	bool pop(){
		if(length == 0){ return false; }
		length -= 1;
		return true;
	}

	T& operator[](usize idx){
		ensure(idx < length, "slice index out of bounds");
		return data[idx];
	}

	T const& operator[](usize idx) const{
		ensure(idx < length, "slice index out of bounds");
		return data[idx];
	}

	Slice<T> take(usize n){
		ensure(n <= length, "cannot take more than length");
		return Slice<T>{ data, n };
	}

	Slice<T> skip(usize n){
		ensure(n <= length, "cannot take more than length");
		return Slice<T>{ &data[n], length - n };
	}

	Slice<T> slice(usize start, usize end){
		ensure(start <= end && end <= length, "invalid slice indices");
		return Slice<T>{ &data[start], end - start };
	}

	void destroy(){
		for(usize i = 0; i < length; i += 1){
			data[i].~T();
		}
		allocator.free(data, layout_of<T>(capacity));
	}
};

template<typename T>
auto make_dynamic_array(usize initial_cap, Allocator alloc){
	Dyn_Array<T> arr = {
		.data = nullptr,
		.length = 0,
		.capacity = 0,
		.allocator = alloc,
	};
	arr.reserve(initial_cap);
	return arr;
}

//// Hash map

// Default hasher overload
template<typename T>
struct hash;

template<Integer T>
struct hash<T>{
	u64 operator()(T k) const {
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

	// Finds internal index for a key with its precomputed hash `h`
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

	bool contains_key(K key){
		auto [_, found] = find(key);
		return found;
	}

	Pair<V, bool> get(K key){
		u64 h = hash_of(key);
		auto [idx, found] = find_with_hash(key, h);
		if(!found){
			return {{}, found};
		}
		return {vals[idx], true};
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

//// Option

// Tag type for constructing empty options, `Option<T> x = None;`
struct None_Type {};
inline constexpr None_Type None{};

// Optional value, the zero value is None (and the payload is zeroed too).
template<typename T>
struct Option {
private:
	union { T data_; };
	bool has_value_;

public:
	constexpr Option() : data_{}, has_value_{false} {}
	constexpr Option(None_Type) : data_{}, has_value_{false} {}
	constexpr Option(T const& value) : data_{value}, has_value_{true} {}


	template<Predicate<T> F>
	constexpr bool is_some_and(F&& pred) const {
		return has_value_ && bool(pred(data_));
	}

	template<Predicate<T> F>
	constexpr bool is_none_or(F&& pred) const {
		return !has_value_ || bool(pred(data_));
	}

	T unwrap(caller_location) const {
		if(!has_value_){ panic("called unwrap() on a None value", caller_loc); }
		return data_;
	}

	T expect(cstring msg, caller_location) const {
		if(!has_value_){ panic(msg, caller_loc); }
		return data_;
	}

	// No check whatsoever, if None this returns whatever is in the payload (the zero value by default)
	constexpr T unwrap_unchecked() const { return data_; }

	constexpr T unwrap_or(T const& fallback) const {
		return has_value_ ? data_ : fallback;
	}

	template<Callable<T> F>
	constexpr T unwrap_or_else(F&& f) const {
		return has_value_ ? data_ : T{f()};
	}

	constexpr T unwrap_or_default() const {
		return has_value_ ? data_ : T{};
	}

	//// Combinators
	template<typename F>
	constexpr auto map(F&& f) const {
		using U = Remove_Reference<decltype(f(data_))>;
		if(has_value_){ return Option<U>(f(data_)); }
		return Option<U>(None);
	}

	template<typename U, Callable<U, T> F>
	constexpr U map_or(U const& fallback, F&& f) const {
		return has_value_ ? U(f(data_)) : fallback;
	}

	template<typename D, typename F>
	constexpr auto map_or_else(D&& fallback, F&& f) const -> decltype(f(data_)) {
		return has_value_ ? f(data_) : fallback();
	}

	// `f` must return an Option<U>
	template<typename F>
	constexpr auto and_then(F&& f) const {
		using R = Remove_Reference<decltype(f(data_))>;
		if(has_value_){ return f(data_); }
		return R(None);
	}

	template<typename U>
	constexpr Option<U> and_(Option<U> const& other) const {
		if(has_value_){ return other; }
		return None;
	}

	// Named `or_` because `or` is an alternative token in C++
	constexpr Option or_(Option const& other) const {
		return has_value_ ? *this : other;
	}

	template<Callable<Option> F>
	constexpr Option or_else(F&& f) const {
		return has_value_ ? *this : Option(f());
	}

	constexpr Option xor_(Option const& other) const {
		if(has_value_ && !other.has_value_){ return *this; }
		if(!has_value_ && other.has_value_){ return other; }
		return None;
	}

	template<Predicate<T> F>
	constexpr Option filter(F&& pred) const {
		if(has_value_ && bool(pred(data_))){ return *this; }
		return None;
	}

	//// Mutation
	// Takes the value out, leaving None (with a zeroed payload) in its place
	constexpr Option take(){
		Option out = *this;
		*this = None;
		return out;
	}

	template<Predicate<T> F>
	constexpr Option take_if(F&& pred){
		if(has_value_ && bool(pred(data_))){ return take(); }
		return None;
	}

	// Puts `value` in, returning the previous contents
	constexpr Option replace(T const& value){
		Option out = *this;
		*this = Option(value);
		return out;
	}

	constexpr T& insert(T const& value){
		*this = Option(value);
		return data_;
	}

	constexpr T& get_or_insert(T const& value){
		if(!has_value_){ *this = Option(value); }
		return data_;
	}

	template<Callable<T> F>
	constexpr T& get_or_insert_with(F&& f){
		if(!has_value_){ *this = Option(T(f())); }
		return data_;
	}

	constexpr T& get_or_insert_default(){
		if(!has_value_){ *this = Option(T{}); }
		return data_;
	}

	attribute_force_inline constexpr bool is_some() const { return has_value_; }
	attribute_force_inline constexpr bool is_none() const { return !has_value_; }
	attribute_force_inline constexpr auto as_ptr() { return has_value_ ? &data_ : nullptr; }
	attribute_force_inline constexpr auto as_ptr() const { return has_value_ ? &data_ : nullptr; }

	constexpr bool operator==(Option const& other) const requires Eq<T> {
		if(has_value_ != other.has_value_){
			return false;
		}
		return !has_value_ || bool(data_ == other.data_);
	}

	constexpr bool operator==(None_Type) const { return !has_value_; }
};

template<typename T>
constexpr Option<T> Some(T const& value){
	return Option<T>(value);
}
