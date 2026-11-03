#include "testing.hpp"
#include "../base.hpp"

namespace test {

static Option<u32> option_digit(char c){
	if(c < '0' || c > '9'){ return option_none(); }
	return option_some(u32(c - '0'));
}

static Option<u32> option_two_digits(cstring s){
	Option<u32> hi = option_digit(s[0]);
	none_return(hi);
	Option<u32> lo = option_digit(s[1]);
	none_return(lo);
	return option_some(hi.unwrap() * 10 + lo.unwrap());
}

static int option_side_effects = 0;
static Option<u32> option_counted_digit(char c){
	option_side_effects += 1;
	return option_digit(c);
}

static Option<u64> option_counted_wide(char c){
	none_return(option_counted_digit(c));
	return option_some(u64(1));
}

// Propagating into an Option with a different value type
static Option<u64> option_sum_pairs(cstring s){
	u64 total = 0;
	for(; s[0] != 0; s += 2){
		Option<u32> n = option_two_digits(s);
		none_return(n);
		total += n.unwrap();
	}
	return option_some(total);
}

// Combinators reject callables that can't take the value
template<typename O, typename F> concept Can_Option_Map = requires(O o, F f){ o.map(f); };
template<typename O, typename F> concept Can_Option_And_Then = requires(O o, F f){ o.and_then(f); };
template<typename O, typename F> concept Can_Option_Unwrap_Or_Else = requires(O o, F f){ o.unwrap_or_else(f); };
template<typename O, typename F> concept Can_Option_Or_Else = requires(O o, F f){ o.or_else(f); };

static_assert(Can_Option_Map<Option<u32>, decltype([](u32 x){ return x; })>);
static_assert(!Can_Option_Map<Option<u32>, decltype([](cstring s){ return s; })>);
static_assert(!Can_Option_Map<Option<u32>, decltype([](u32){})>);
static_assert(Can_Option_Unwrap_Or_Else<Option<u32>, decltype([]{ return 1u; })>);
static_assert(!Can_Option_Unwrap_Or_Else<Option<u32>, decltype([]{ return "not a u32"; })>);
static_assert(!Can_Option_Unwrap_Or_Else<Option<u32>, decltype([](u32 x){ return x; })>);
static_assert(Can_Option_Or_Else<Option<u32>, decltype([]{ return Option<u32>{}; })>);
static_assert(!Can_Option_Or_Else<Option<u32>, decltype([]{ return 1u; })>);

bool option_tests(){
	bool ok = true;
	ok &= test("option: zero value", [](Test& t){
		Option<u32> o = {};
		check(!o.has_value());
		check(o.unwrap_or(5) == 5);
		// The union is zeroed when empty
		check(o.unwrap_unchecked() == 0);

		Option<u64> wide = {};
		check(!wide.has_value() && wide.unwrap_unchecked() == 0);

		Option<cstring> s = {};
		check(!s.has_value() && s.unwrap_unchecked() == nullptr);
	});

	ok &= test("option: some and none", [](Test& t){
		Option<u32> a = option_digit('7');
		check(a.has_value() && a.unwrap() == 7);

		Option<u32> b = option_digit('x');
		check(!b.has_value());
		check(b.unwrap_or(42) == 42);
		check(a.unwrap_or(42) == 7);

		// Zero is a value, not nothing
		Option<u32> zero = option_digit('0');
		check(zero.has_value() && zero.unwrap() == 0);

		// Tags convert when the value type is merely convertible
		Option<u64> wide = option_some(u32(9));
		check(wide.has_value() && wide.unwrap() == 9);
		Option<cstring> none = option_none();
		check(!none.has_value());
	});

	ok &= test("option: none_return", [](Test& t){
		Option<u32> r = option_two_digits("42");
		check(r.has_value() && r.unwrap() == 42);

		check(!option_two_digits("x2").has_value());
		check(!option_two_digits("4x").has_value());

		// Argument is evaluated exactly once
		option_side_effects = 0;
		check(option_counted_wide('3').has_value());
		check(!option_counted_wide('z').has_value());
		check(option_side_effects == 2);

		check(option_sum_pairs("102030").unwrap() == 60);
		check(!option_sum_pairs("1020x0").has_value());
	});

	ok &= test("option: unwrap and expect", [](Test& t){
		Option<u32> a = option_digit('4');
		Option<u32> b = option_digit('?');
		check(a.unwrap() == 4);
		check(a.expect("digit") == 4);

		int calls = 0;
		check(a.unwrap_or_else([&]{ calls += 1; return 0u; }) == 4);
		check(b.unwrap_or_else([&]{ calls += 1; return 100u; }) == 100);
		check(calls == 1);
	});

	ok &= test("option: map", [](Test& t){
		Option<u32> a = option_digit('4');
		Option<u32> b = option_digit('?');

		Option<u64> wide = a.map([](u32 x){ return u64(x) << 40; });
		check(wide.has_value() && wide.unwrap() == (u64(4) << 40));

		int calls = 0;
		Option<u64> wide_none = b.map([&](u32 x){ calls += 1; return u64(x); });
		check(!wide_none.has_value() && calls == 0);

		Option<cstring> msg = a.map([](u32){ return "digit"; });
		check(msg.has_value() && msg.unwrap() != nullptr);
		check(a.map([](u32 x){ return x + 1; }).map([](u32 x){ return x * 2; }).unwrap() == 10);
	});

	ok &= test("option: and_then and or_else", [](Test& t){
		auto double_digit = [](u32 x) -> Option<u32> {
			if(x * 2 > 9){ return option_none(); }
			return option_some(x * 2);
		};

		check(option_digit('3').and_then(double_digit).unwrap() == 6);
		check(!option_digit('7').and_then(double_digit).has_value());
		check(!option_digit('x').and_then(double_digit).has_value());
		check(option_digit('2').and_then(double_digit).and_then(double_digit).unwrap() == 8);

		int calls = 0;
		auto fallback = [&]() -> Option<u32> { calls += 1; return option_some(0u); };
		check(option_digit('x').or_else(fallback).unwrap() == 0);
		check(option_digit('5').or_else(fallback).unwrap() == 5);
		check(calls == 1);

		auto still_none = []() -> Option<u32> { return option_none(); };
		check(!option_digit('x').or_else(still_none).has_value());

		// and_then may change the value type
		Option<cstring> s = option_digit('1').and_then([](u32) -> Option<cstring> { return option_some("one"); });
		check(s.has_value() && s.unwrap() != nullptr);
	});
	return ok;
}

}
