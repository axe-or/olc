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
	#define attribute_force_inline_func __forceinline static
#elif defined(__clang__) || defined(__GNUC__)
	#define attribute_force_inline      __attribute__((always_inline))
	#define attribute_force_inline_func __attribute__((always_inline)) static inline
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
#include <stdatomic.h>

//// Basic types & Utilities
typedef int8_t i8;
typedef uint8_t u8;

typedef int16_t i16;
typedef uint16_t u16;

typedef int32_t i32;
typedef uint32_t u32;

typedef int64_t i64;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

typedef int32_t rune;
typedef uintptr_t uintptr;

typedef size_t    usize;
typedef ptrdiff_t isize;

typedef const char* cstring;

typedef _Atomic(int) AtomicInt;
typedef _Atomic(bool) AtomicBool;

#define min(x, y) (((x) < (y)) ? (x) : (y))

#define max(x, y) (((x) > (y)) ? (x) : (y))

#define clamp(lo, x, hi) min(max((lo), (x)), (hi))

// Helpers that use preprocessor expansion tricks to "glue" identifiers
#define ident_concat0(x, y) x##y
#define ident_concat1(x, y) ident_concat0(x, y)
#define ident_concat2(x, y) ident_concat1(x, y)
#define ident_concat(x, y)  ident_concat2(x, y)
#define ident_counter(x)    ident_concat(x, __COUNTER__)

//// Assertions

// Exit the program fatally
_Noreturn void trap();

_Noreturn void panic_ex(cstring msg, cstring filename, int line);

bool ensure_ex(bool pred, cstring msg, cstring filename, int line);

// Exit the program fatally with a message
#define panic(msg) panic_ex((msg), __FILE__, __LINE__)

// Exit the program fatally with a message if a predicate fails
#define ensure(pred, msg) ensure_ex((pred), (msg), __FILE__, __LINE__)

// Same as `ensure` but only when BUILD_DEBUG is defined
#if defined(BUILD_DEBUG)
	#define ensure_debug(pred, msg) ensure_ex((pred), (msg), __FILE__, __LINE__)
#else
	#define ensure_debug(pred, msg)
#endif

// Helper for unimplemented sections of code
#define TODO() panic_ex("TODO", __FILE__, __LINE__)

//// String

// UTF-8 encoded slice of bytes
typedef struct {
	u8 const* v;
	isize len;
} String;

// Helper macro to turn usual C-strings into sized strings
#define $str(s) ((String){.v = (u8 const*)("" s ""), .len = (sizeof(s) - 1)})

// Helper macro to use sized string with printf's "%.*s"
#define $strfmt(S) ((int)((S).len)), ((u8 const*)((S).v))

// Length of a C-style string
static inline
isize cstring_len(cstring cs) {
	isize n = 0;
	while(cs[n] != 0){
		n += 1;
	}
	return n;
}

// The error unicode codepoint
#define RUNE_ERROR ((rune)0xfffd)

// Decoded form of a unicode codepoint
typedef struct {
	rune codepoint;
	i32  size;
} RuneDecoded;

// Encoded form of a unicode codepoint
typedef struct {
	u8  bytes[4];
	i32 size;
} RuneEncoded;

// Encode a codepoint `r` to UTF-8
RuneEncoded rune_encode(rune r);

// Decode the first rune of a UTF-8 encoded buffer
RuneDecoded rune_decode(u8 const* buf, u32 buflen);

//// Slice
#define Slice(T) struct { T* v; isize len; }

#define slice_len(S) ((S).len)

#define slice_take(S, N) \
	(ensure_ex((N) <= (S).len, "cannot take more than slice length", __FILE__, __LINE__) \
	? (typeof(S)){ .v = (S).v, .len = (N) } \
	: (typeof(S)){0})

#define slice_skip(S, N) \
	(ensure_ex((N) <= (S).len, "cannot skip more than slice length", __FILE__, __LINE__) \
	? (typeof(S)){ .v = &(S).v[(N)], .len = (S).len - (N) } \
	: (typeof(S)){0})

#define slice(S, A, B) \
	(ensure_ex((B) <= (S).len && (A) <= (B), "invalid slice indices", __FILE__, __LINE__) \
	? (typeof(S)){ .v = &(S).v[(A)], .len = (B) - (A) } \
	: (typeof(S)){0})
