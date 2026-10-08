#include "testing.hpp"
#include "../base.hpp"

namespace test {

static constexpr u64 const_hash(char const* s){
	u8 buf[64] = {};
	usize n = cstring_len(s);
	for(usize i = 0; i < n; i++){ buf[i] = u8(s[i]); }
	return hash_murmur64(Slice<u8>{buf, n});
}

// MurmurHash64A of the empty input with seed zero is zero
static_assert(const_hash("") == 0);
static_assert(const_hash("hello") == const_hash("hello"));
static_assert(const_hash("hello") != const_hash("hellp"));
static_assert(cstring_len("") == 0 && cstring_len("abc") == 3);
static_assert(String("abc") == String("abc") && String("abc") != String("abd"));

void string_tests(){
	test("string: construction and equality", [](Test& t){
		String empty;
		check(empty.len() == 0 && empty.raw_data() == nullptr);

		char const* cs = "hello";
		String s = cs;
		check(s.len() == 5 && s.raw_data() == cs);
		check(s[0] == 'h' && s[4] == 'o');

		String sized{"hello world", 5};
		check(sized.len() == 5);
		check(sized == s);
		check(!(sized != s));
		check(String("hell") != s);
		check(String("hellO") != s);
		check(String("") == empty);
	});

	test("string: take, skip, slice", [](Test& t){
		String s = "hello world";
		check(s.take(5) == String("hello"));
		check(s.skip(6) == String("world"));
		check(s.slice(2, 4) == String("ll"));
		check(s.slice(3, 3).len() == 0);
		check(s.take(11) == s && s.skip(11).len() == 0);
	});

	test("string: hash", [](Test& t){
		String a = "some key";
		String b{"some key and more", 8};
		check(a.hash() == b.hash());
		check(a.hash() == const_hash("some key"));
		check(String("").hash() == 0);
		check(String("abc").hash() != String("abd").hash());
		// Exercises both the 8 byte block loop and the tail
		check(String("0123456789abcdef!").hash() == const_hash("0123456789abcdef!"));
	});
}

}
