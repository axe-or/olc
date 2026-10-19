#include "testing.hpp"
#include "../base.hpp"

static_assert(min_val<i8> == INT8_MIN && max_val<i8> == INT8_MAX);
static_assert(min_val<i16> == INT16_MIN && max_val<i16> == INT16_MAX);
static_assert(min_val<i32> == INT32_MIN && max_val<i32> == INT32_MAX);
static_assert(min_val<i64> == INT64_MIN && max_val<i64> == INT64_MAX);

static_assert(min_val<u8> == 0 && max_val<u8> == UINT8_MAX);
static_assert(min_val<u16> == 0 && max_val<u16> == UINT16_MAX);
static_assert(min_val<u32> == 0 && max_val<u32> == UINT32_MAX);
static_assert(min_val<u64> == 0 && max_val<u64> == UINT64_MAX);

static_assert(min_val<usize> == 0 && max_val<usize> == SIZE_MAX);
static_assert(min_val<isize> == PTRDIFF_MIN && max_val<isize> == PTRDIFF_MAX);

static_assert(min_val<char> == (char(-1) < 0 ? -128 : 0));
static_assert(max_val<char> == (char(-1) < 0 ? 127 : 255));
static_assert(min_val<long> < 0 && max_val<long> == ~0ul >> 1);
static_assert(max_val<unsigned long long> == ~0ull);
