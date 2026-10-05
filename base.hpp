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
		ensure(start <= len_ && end >= start, "invalid slice indices");
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
		ensure(start <= len_ && end >= start, "invalid slice indices");
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
		return (void*)(proc_(impl_, Mem_Grow, ptr, old, desired));
	}

	attribute_force_inline void* shrink(void* ptr, Memory_Layout old, Memory_Layout desired) const {
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

Allocator heap_allocator();

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

		data = p;
		capacity = desired;
		return true;
	}

	bool shrink_to_fit(){
		void* p = allocator.shrink(data, layout_of<T>(length));
		if(!p){ return false; }
		data = p;
		capacity = length;
	}

	// Insert value at `idx`, shifting all elements
	bool insert(usize idx, T const& val){
		if(idx > length){ return false; }
		if(length == capacity){
			usize desired = max(16, capacity * 2);
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
		ensure(start <= length && end >= start, "invalid slice indices");
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
