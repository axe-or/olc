#include "testing.hpp"
#include "../base.hpp"

namespace test {

enum class Parse_Error : u8 {
	None = 0,
	Empty,
	Not_A_Digit,
};

static Result<u32, Parse_Error> parse_digit(char c){
	if(c == 0){ return result_err(Parse_Error::Empty); }
	if(c < '0' || c > '9'){ return result_err(Parse_Error::Not_A_Digit); }
	return result_ok(u32(c - '0'));
}

static Result<u32, Parse_Error> parse_two_digits(cstring s){
	Result<u32, Parse_Error> hi = parse_digit(s[0]);
	err_return(hi);
	Result<u32, Parse_Error> lo = parse_digit(s[1]);
	err_return(lo);
	return result_ok(hi.unwrap() * 10 + lo.unwrap());
}

static Result<void, Parse_Error> validate_digits(cstring s){
	err_return(parse_two_digits(s));
	return result_ok();
}

static int side_effects = 0;
static Result<u32, Parse_Error> counted_digit(char c){
	side_effects += 1;
	return parse_digit(c);
}

static Result<void, Parse_Error> validate_counted(char c){
	err_return(counted_digit(c));
	return result_ok();
}

static Result<u32, cstring> parse_or_message(cstring s){
	Result<u32, Parse_Error> r = parse_two_digits(s);
	if(!r.has_value()){ return result_err("not a number"); }
	return result_ok(r.unwrap());
}

// Propagating into a Result with a different value type
static Result<u64, Parse_Error> sum_pairs(cstring s){
	u64 total = 0;
	for(; s[0] != 0; s += 2){
		Result<u32, Parse_Error> n = parse_two_digits(s);
		err_return(n);
		total += n.unwrap();
	}
	return result_ok(total);
}

// Combinators reject callables that can't take the value/error
template<typename R, typename F> concept Can_Map = requires(R r, F f){ r.map(f); };
template<typename R, typename F> concept Can_Map_Err = requires(R r, F f){ r.map_err(f); };
template<typename R, typename F> concept Can_And_Then = requires(R r, F f){ r.and_then(f); };
template<typename R, typename F> concept Can_Unwrap_Or_Else = requires(R r, F f){ r.unwrap_or_else(f); };

static_assert(Can_Map<Result<u32, Parse_Error>, decltype([](u32 x){ return x; })>);
static_assert(!Can_Map<Result<u32, Parse_Error>, decltype([](cstring s){ return s; })>);
static_assert(!Can_Map_Err<Result<u32, Parse_Error>, decltype([](u32* p){ return p; })>);
static_assert(!Can_Unwrap_Or_Else<Result<u32, Parse_Error>, decltype([](Parse_Error){ return "not a u32"; })>);
static_assert(!Can_And_Then<Result<void, Parse_Error>, decltype([](u32 x){ return x; })>);

bool result_tests(){
	bool ok = true;
	ok &= test("result: zero value", [](Test& t){
		Result<u32, Parse_Error> r = {};
		check(!r.has_value() && r.unwrap_err() == Parse_Error::None);
		check(r.unwrap_or(5) == 5);

		Result<void, Parse_Error> v = {};
		check(!v.has_value() && v.unwrap_err() == Parse_Error::None);

		Result<u64, cstring> s = {};
		check(!s.has_value() && s.unwrap_err() == nullptr);

		// The union zeroes `error`, which is the larger member here
		Result<u8, u64> wide = {};
		check(!wide.has_value() && wide.unwrap_err() == 0);
	});

	ok &= test("result: value and error", [](Test& t){
		Result<u32, Parse_Error> a = parse_digit('7');
		check(a.has_value() && a.unwrap() == 7);

		Result<u32, Parse_Error> b = parse_digit('x');
		check(!b.has_value() && b.unwrap_err() == Parse_Error::Not_A_Digit);
		check(b.unwrap_or(42) == 42);
		check(a.unwrap_or(42) == 7);

		Result<u32, cstring> m = parse_or_message("a1");
		check(!m.has_value() && m.unwrap_err() != nullptr);
		check(parse_or_message("12").unwrap() == 12);
	});

	ok &= test("result: err_return", [](Test& t){
		Result<u32, Parse_Error> r = parse_two_digits("42");
		check(r.has_value() && r.unwrap() == 42);

		r = parse_two_digits("x2");
		check(!r.has_value() && r.unwrap_err() == Parse_Error::Not_A_Digit);

		r = parse_two_digits("4");
		check(!r.has_value() && r.unwrap_err() == Parse_Error::Empty);

		check(validate_digits("99").has_value());
		Result<void, Parse_Error> v = validate_digits("9?");
		check(!v.has_value() && v.unwrap_err() == Parse_Error::Not_A_Digit);

		// Argument is evaluated exactly once
		side_effects = 0;
		check(validate_counted('3').has_value());
		check(!validate_counted('z').has_value());
		check(side_effects == 2);

		check(sum_pairs("102030").unwrap() == 60);
		Result<u64, Parse_Error> sum = sum_pairs("1020x0");
		check(!sum.has_value() && sum.unwrap_err() == Parse_Error::Not_A_Digit);
	});
	ok &= test("result: unwrap and expect", [](Test& t){
		Result<u32, Parse_Error> a = parse_digit('4');
		Result<u32, Parse_Error> b = parse_digit('?');
		check(a.expect("digit") == 4);
		check(b.unwrap_err() == Parse_Error::Not_A_Digit);
		check(b.expect_err("not a digit") == Parse_Error::Not_A_Digit);

		Result<void, Parse_Error> v = validate_digits("a1");
		check(v.unwrap_err() == Parse_Error::Not_A_Digit);
		validate_digits("11").expect("valid");

		int calls = 0;
		check(a.unwrap_or_else([&](Parse_Error){ calls += 1; return 0u; }) == 4);
		check(b.unwrap_or_else([&](Parse_Error e){ calls += 1; return u32(e) + 100; }) == 102);
		check(calls == 1);
	});

	ok &= test("result: map and map_err", [](Test& t){
		Result<u32, Parse_Error> a = parse_digit('4');
		Result<u32, Parse_Error> b = parse_digit('?');

		Result<u64, Parse_Error> wide = a.map([](u32 x){ return u64(x) << 40; });
		check(wide.has_value() && wide.unwrap() == (u64(4) << 40));

		int calls = 0;
		Result<u64, Parse_Error> wide_err = b.map([&](u32 x){ calls += 1; return u64(x); });
		check(!wide_err.has_value() && wide_err.unwrap_err() == Parse_Error::Not_A_Digit && calls == 0);

		Result<void, Parse_Error> nothing = a.map([&](u32){ calls += 1; });
		check(nothing.has_value() && calls == 1);

		Result<u32, cstring> msg = b.map_err([](Parse_Error){ return "bad digit"; });
		check(!msg.has_value() && msg.unwrap_err() != nullptr);
		check(a.map_err([](Parse_Error){ return "bad digit"; }).unwrap() == 4);

		Result<void, Parse_Error> v = validate_digits("?1");
		check(v.map_err([](Parse_Error e){ return u32(e); }).unwrap_err() == u32(Parse_Error::Not_A_Digit));
		check(validate_digits("11").map([]{ return 7; }).unwrap() == 7);
	});

	ok &= test("result: and_then and or_else", [](Test& t){
		auto double_digit = [](u32 x) -> Result<u32, Parse_Error> {
			if(x * 2 > 9){ return result_err(Parse_Error::Not_A_Digit); }
			return result_ok(x * 2);
		};

		check(parse_digit('3').and_then(double_digit).unwrap() == 6);
		check(parse_digit('7').and_then(double_digit).unwrap_err() == Parse_Error::Not_A_Digit);
		check(parse_digit(0).and_then(double_digit).unwrap_err() == Parse_Error::Empty);
		check(parse_digit('2').and_then(double_digit).and_then(double_digit).unwrap() == 8);

		auto default_zero = [](Parse_Error e) -> Result<u32, cstring> {
			if(e == Parse_Error::Empty){ return result_ok(0); }
			return result_err("unrecoverable");
		};
		check(parse_digit(0).or_else(default_zero).unwrap() == 0);
		check(parse_digit('x').or_else(default_zero).unwrap_err() != nullptr);
		check(parse_digit('5').or_else(default_zero).unwrap() == 5);

		Result<void, Parse_Error> v = validate_digits("12").and_then([]() -> Result<void, Parse_Error> {
			return result_err(Parse_Error::Empty);
		});
		check(v.unwrap_err() == Parse_Error::Empty);
		check(validate_digits("x").or_else([](Parse_Error) -> Result<void, int> { return result_ok(); }).has_value());
	});
	return ok;
}

}
