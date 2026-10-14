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
#if !defined(BUILD_PLATFORM_WASI)
	// The WASI sysroot ships without libc++ headers
	#include <atomic>
#endif

template<typename To, typename From>
constexpr To bit_cast(const From& from){
	return __builtin_bit_cast(To, from);
}

struct Source_Location {
	const char* file_;
	unsigned line_;

	static constexpr Source_Location current(const char* file = __builtin_FILE(), unsigned line = __builtin_LINE()){
		return Source_Location{file, line};
	}

	constexpr const char* file_name() const { return file_; }
	constexpr unsigned line() const { return line_; }
};

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

#if !defined(BUILD_PLATFORM_WASI)
template<typename T>
using Atomic = std::atomic<T>;
#endif

#define caller_location Source_Location caller_loc = Source_Location::current()

#define min(x, y) (((x) < (y)) ? (x) : (y))

#define max(x, y) (((x) > (y)) ? (x) : (y))

#define clamp(lo, x, hi) min(max((lo), (x)), (hi))

#define attr_get(name, expr) attribute_force_inline constexpr auto name () const { return expr ; }

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
	#define ensure_debug(pred, msg) ensure((pred), (msg), Source_Location::current())
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

// `F` can be called with `Args...` irrespective of its return type
template<typename F, typename ... Args>
concept Callable_With = requires(F f, Args... args){
	f(args...);
};

template<typename F, typename ... Args>
concept Predicate = Callable<F, bool, Args...>;

// Type returned by calling `F` with `Args...`
template<typename F, typename ... Args>
using Call_Result = decltype(detail::declval<F>(0)(detail::declval<Args>(0)...));

//// Result

// Tags produced by result_ok() / result_err(), they convert to any Result with
// compatible types, this is an implicit conversion trick, it should be avoided
// but is permitted in this case because of significant improvements to
// ergonomics
template<typename T>
struct Result_Ok { T value; };

template<>
struct Result_Ok<void> {};

template<typename E>
struct Result_Err { E error; };

template<typename T>
constexpr Result_Ok<T> result_ok(T value){ return {value}; }

constexpr Result_Ok<void> result_ok(){ return {}; }

template<typename E>
constexpr Result_Err<E> result_err(E error){ return {error}; }

// Either a value or an error
template<typename T, typename E>
struct [[nodiscard]] Result {
private:
	bool has_value_ = false;
	union {
		E error_ = {};
		T value_;
	};

public:
	constexpr Result() = default;

	template<Convertible_To<T> U>
	constexpr Result(Result_Ok<U> ok) : has_value_{true}, value_(ok.value){}

	template<Convertible_To<E> U>
	constexpr Result(Result_Err<U> err) : has_value_{false}, error_(err.error){}

	attribute_force_inline constexpr bool has_value() const { return has_value_; }

	T unwrap(caller_location) const {
		if(!has_value_){
			panic("unwrap() on error result", caller_loc);
		}
		return value_;
	}

	T unwrap_unchecked() const {
		return value_;
	}

	E unwrap_err(caller_location) const {
		if(has_value_){
			panic("unwrap_err() on result with a value", caller_loc);
		}
		return error_;
	}

	E unwrap_err_unchecked() const {
		return error_;
	}

	T expect(cstring msg, caller_location) const {
		if(!has_value_){
			panic(msg, caller_loc);
		}
		return value_;
	}

	E expect_err(cstring msg, caller_location) const {
		if(has_value_){
			panic(msg, caller_loc);
		}
		return error_;
	}

	T unwrap_or(T fallback) const {
		return has_value_ ? value_ : fallback;
	}

	// `f(error)` computes the fallback, only called when there is no value
	template<Callable<T, E> F>
	T unwrap_or_else(F&& f) const {
		return has_value_ ? value_ : f(error_);
	}

	// Result<U, E> with `f(value)` as the value, errors pass through untouched
	template<Callable_With<T> F>
	Result<Call_Result<F, T>, E> map(F&& f) const {
		if(!has_value_){ return result_err(error_); }
		if constexpr(detail::is_void<Call_Result<F, T>>){
			f(value_);
			return result_ok();
		}
		else {
			return result_ok(f(value_));
		}
	}

	// Result<T, E2> with `f(error)` as the error, values pass through untouched
	template<Callable_With<E> F>
	Result<T, Call_Result<F, E>> map_err(F&& f) const  {
		if(has_value_){ return result_ok(value_); }
		return result_err(f(error_));
	}

	// Chain a fallible operation: `f(value)` returns a Result, only called when there is a value
	template<Callable_With<T> F>
	Call_Result<F, T> and_then(F&& f) {
		if(!has_value_){ return result_err(error_); }
		return f(value_);
	}

	// Recover from an error: `f(error)` returns a Result, only called when there is no value
	template<Callable_With<E> F>
	Call_Result<F, E> or_else(F&& f) {
		if(has_value_){ return result_ok(value_); }
		return f(error_);
	}
};

// Result of an operation that only produces an error
template<typename E>
struct [[nodiscard]] Result<void, E> {
private:
	bool has_value_ = false;
	E error_ = {};

public:
	constexpr Result() = default;

	constexpr Result(Result_Ok<void>) : has_value_{true}{}

	template<Convertible_To<E> U>
	constexpr Result(Result_Err<U> err) : has_value_{false}, error_(err.error){}

	attribute_force_inline constexpr bool has_value() const { return has_value_; }

	void unwrap(caller_location) const {
		ensure(has_value_, "unwrap() on error result", caller_loc);
	}

	E unwrap_err(caller_location) const {
		ensure(!has_value_, "unwrap_err() on result with a value", caller_loc);
		return error_;
	}

	E unwrap_err_unchecked() const {
		return error_;
	}

	void expect(cstring msg, caller_location) const {
		ensure(has_value_, msg, caller_loc);
	}

	E expect_err(cstring msg, caller_location) const {
		ensure(!has_value_, msg, caller_loc);
		return error_;
	}

	// Result<U, E> with `f()` as the value, errors pass through untouched
	template<typename F> requires Callable<F, Call_Result<F>>
	auto map(F&& f) const -> Result<Call_Result<F>, E> {
		if(!has_value_){ return result_err(error_); }
		if constexpr(detail::is_void<Call_Result<F>>){
			f();
			return result_ok();
		}
		else {
			return result_ok(f());
		}
	}

	template<typename F> requires Callable<F, Call_Result<F, E>, E>
	auto map_err(F&& f) const -> Result<void, Call_Result<F, E>> {
		if(has_value_){ return result_ok(); }
		return result_err(f(error_));
	}

	template<typename F> requires Callable<F, Call_Result<F>>
	auto and_then(F&& f) const -> Call_Result<F> {
		if(!has_value_){ return result_err(error_); }
		return f();
	}

	template<typename F> requires Callable<F, Call_Result<F, E>, E>
	auto or_else(F&& f) const -> Call_Result<F, E> {
		if(has_value_){ return result_ok(); }
		return f(error_);
	}
};

// Return the error of `res` from the enclosing function if it has one, `res` is
// evaluated once. The enclosing function must return a Result whose error type
#define err_return(res) do { \
	auto const& err_return_res_ = (res); \
	if(!err_return_res_.has_value()){ return result_err(err_return_res_.unwrap_err_unchecked()); } \
} while(0)

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
#define STRF(S) ((int)((S).len())), ((S).raw_data())

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
	void* memmove(void*, void const*, size_t) noexcept;
	void* memcpy(void*, void const*, size_t) noexcept;
	void* memset(void*, int, size_t) noexcept;
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

//// IO interface

enum IO_Error : i32 {
	IO_Error_None = 0,

	IO_Error_EOF = -1,
	IO_Error_Closed = -2,

	IO_Error_Other = -4096,
};

enum IO_Mode : u8 {
	IO_Query = 0,

	IO_Read  = 1 << 0,
	IO_Write = 1 << 1,
};

using IO_Func = i64 (*)(void* impl, IO_Mode mode, Slice<u8> buf);

struct IO_Stream {
	void* impl_;
	IO_Func fn_;

	// Read data into buffer, returns written bytes, or a number < 0 on error
	i64 read(Slice<u8> buf){
		return fn_(impl_, IO_Read, buf);
	}

	// Write data from buffer, returns written bytes, or a number < 0 on error
	i64 write(Slice<u8> buf){
		return fn_(impl_, IO_Write, buf);
	}

	// Get supported operations from stream
	u8 query(){
		return (u8)fn_(impl_, IO_Query, {});
	}
};

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
constexpr auto layout_of(){
	return Memory_Layout{ sizeof(T) , alignof(T) };
}

template<typename T>
constexpr auto layout_of_array(usize count){
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

	template<typename T, typename ...Args>
	T* make(Args&& ...args){
		void* storage = alloc(layout_of<T>());
		if(storage == nullptr){ return nullptr; }
		return new (New_Tag{}, storage) T(::forward<Args>(args)...);
	}

template<typename T, typename ...Args>
Slice<T> slice_make(Allocator a, usize count, Args&& ...args){
	T* p = static_cast<T*>(a.alloc(layout_of_array<T>(count)));
	if(p == nullptr){ return {}; }

	for(usize i = 0; i < count; i += 1){
		new (New_Tag{}, p + i) T(::forward<Args>(args)...);
	}
	return Slice<T>{ p, count };
}


// Allocator that always fails allocations
Allocator null_allocator();

// Allocator that instantly explodes if invoked
Allocator panic_allocator();

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
		auto [size, align] = layout_of<T>();
		void* p = alloc(size, align);
		if(!p){
			return nullptr;
		}

		return new (New_Tag{}, p) T(::forward<Args>(args)...);
	}

	template<typename T, typename ...Args>
	Slice<T> slice_make(usize count, Args&& ...args){
		auto [size, align] = layout_of_array<T>(count);
		T* p = static_cast<T*>(alloc(size, align));
		if(!p){
			return {};
		}

		for(usize i = 0; i < count; i += 1){
			new (New_Tag{}, p + i) T(::forward<Args>(args)...);
		}
		return Slice<T>{ p, count };
	}
};

Arena arena_from_buffer(Slice<u8> buffer);

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
	T* data_;
	usize length_;
	usize capacity_;
	Allocator allocator_;

	bool reserve(usize desired){
		if(capacity_ >= desired){
			return true;
		}

		void* p = allocator_.grow(data_, layout_of_array<T>(capacity_), layout_of_array<T>(desired));
		if(!p){
			return false;
		}

		data_ = (T*)p;
		capacity_ = desired;
		return true;
	}

	bool shrink_to_fit(){
		void* p = allocator_.shrink(data_, layout_of_array<T>(capacity_), layout_of_array<T>(length_));
		if(!p){ return false; }
		data_ = (T*)p;
		capacity_ = length_;
		return true;
	}

	// Insert value at `idx`, shifting all elements
	bool insert(usize idx, T const& val){
		if(idx > length_){ return false; }
		if(length_ == capacity_){
			usize desired = max(usize(16), capacity_ * 2);
			if(desired < capacity_ || !reserve(desired)){ return false; }
		}
		for(usize i = length_; i > idx; i -= 1){
			data_[i] = data_[i - 1];
		}
		data_[idx] = val;
		length_ += 1;
		return true;
	}

	// Insert value at `idx` by swapping the value at position with the last value
	bool insert_swap(usize idx, T const& val){
		if(idx > length_){ return false; }
		if(length_ == capacity_){
			usize desired = capacity_ == 0 ? 8 : capacity_ * 2;
			if(desired < capacity_ || !reserve(desired)){
				return false;
			}
		}

		if(idx < length_){
			data_[length_] = data_[idx];
		}

		data_[idx] = val;
		length_ += 1;
		return true;
	}

	// Remove value `idx`, shifting all elements
	bool remove(usize idx){
		if(idx >= length_){ return false; }
		for(usize i = idx; i + 1 < length_; i += 1){
			data_[i] = data_[i + 1];
		}
		length_ -= 1;
		return true;
	}

	// Remove value at `idx` by swapping the value at position with the last value
	bool remove_swap(usize idx){
		if(idx >= length_){ return false; }
		length_ -= 1;
		if(idx < length_){
			data_[idx] = data_[length_];
		}
		return true;
	}

	// Push item to end of array
	bool append(T const& val){
		return insert_swap(length_, val);
	}

	// Pop last item of array
	bool pop(){
		if(length_ == 0){ return false; }
		length_ -= 1;
		return true;
	}

	T& operator[](usize idx){
		ensure(idx < length_, "slice index out of bounds");
		return data_[idx];
	}

	T const& operator[](usize idx) const{
		ensure(idx < length_, "slice index out of bounds");
		return data_[idx];
	}

	Slice<T> take(usize n){
		ensure(n <= length_, "cannot take more than length");
		return Slice<T>{ data_, n };
	}

	Slice<T> skip(usize n){
		ensure(n <= length_, "cannot take more than length");
		return Slice<T>{ &data_[n], length_ - n };
	}

	Slice<T> slice(usize start, usize end){
		ensure(start <= end && end <= length_, "invalid slice indices");
		return Slice<T>{ &data_[start], end - start };
	}

	void destroy(){
		for(usize i = 0; i < length_; i += 1){
			data_[i].~T();
		}
		allocator_.free(data_, layout_of_array<T>(capacity_));
	}

	void clear(){
		length_ = 0;
	}

	attr_get(len, length_);
	attr_get(cap, capacity_);
	attr_get(raw_data, data_);
};

template<typename T>
auto dynamic_array_make(usize initial_cap, Allocator alloc){
	Dyn_Array<T> arr = {
		.data_ = nullptr,
		.length_ = 0,
		.capacity_ = 0,
		.allocator_ = alloc,
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
		Arena arena = arena_from_buffer({(u8*)block, layout.size});
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
auto map_make(usize capacity, Allocator a){
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

//// Typed arena

template<typename T, Unsigned_Integer I = u32>
struct Typed_Arena_ID {
	I v;

	constexpr bool operator!(){ return !v; }

	constexpr auto operator<=>(Typed_Arena_ID<T, I> const&) const = default;
};

template<typename T, Unsigned_Integer I = u32>
struct Typed_Arena {
	using ID = Typed_Arena_ID<T, I>;
	Dyn_Array<T> storage;

	T& operator[](ID id){
		ensure(id.v > 0 && id.v < storage.len(), "invalid ID");
		return storage[id - 1];
	}

	T const& operator[](ID id) const {
		ensure(id.v > 0 && id.v < storage.len(), "invalid ID");
		return storage[id.v - 1];
	}

	T* get(ID id){
		if(id.v == 0 || id.v >= storage.len()){
			return nullptr;
		}
		return &storage[id.v - 1];
	}

	T const* get(ID id) const {
		if(id.v == 0 || id.v >= storage.len()){
			return nullptr;
		}
		return &storage[id.v - 1];
	}

	ID put(T const& val){
		bool ok = storage.append(val);
		if(!ok){ return 0; }
		return ID{storage.len()};
	}

	void reset(){
		storage.clear();
	}

	void destroy(){
		storage.destroy();
	}
};

template<typename T, Unsigned_Integer I = u32>
auto make_typed_arena(usize capacity, Allocator a){
	return Typed_Arena<T, I> {
		.storage = dynamic_array_make<T>(capacity, a),
	};
}

#undef attr_get
