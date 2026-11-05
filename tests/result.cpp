#include "testing.hpp"
#include "../base.hpp"
#include "../compiler.hpp"

namespace test {

enum class Parse_Error : u8 {
	None = 0,
	Empty,
	Not_A_Digit,
};

enum Plain_Error {
	Plain_None = 0,
	Plain_Bad = 7,
	Plain_Negative = -3,
};

// Brings its own notion of error
struct Own_Error {
	int code;
	bool is_error() const { return code >= 100; }
};

struct Not_An_Error { int x; };

// Anything that `is_error` can't classify is rejected
template<typename T> concept Can_Is_Error = requires(T const& v){ is_error(v); };

static_assert(Can_Is_Error<int>);
static_assert(Can_Is_Error<u8>);
static_assert(Can_Is_Error<i64>);
static_assert(Can_Is_Error<bool>);
static_assert(Can_Is_Error<Parse_Error>);
static_assert(Can_Is_Error<Plain_Error>);
static_assert(Can_Is_Error<Own_Error>);
static_assert(Can_Is_Error<Syntax_Error>);
static_assert(Can_Is_Error<IO_Error>);
static_assert(!Can_Is_Error<float>);
static_assert(!Can_Is_Error<double>);
static_assert(!Can_Is_Error<int*>);
static_assert(!Can_Is_Error<cstring>);
static_assert(!Can_Is_Error<Not_An_Error>);
static_assert(!Can_Is_Error<String>);

// Result<void, E> is the bare error, any other Result is a { value, error } pair
static_assert(Same_As<Result<void, Parse_Error>, Parse_Error>);
static_assert(Same_As<Result<void, bool>, bool>);
static_assert(!Same_As<Result<u32, Parse_Error>, Parse_Error>);
static_assert(!Can_Is_Error<Result<u32, Parse_Error>>);

// The error type of a Result must be something `is_error` accepts
template<typename T, typename E> concept Can_Form_Result = requires{ typename Result<T, E>; };

static_assert(Can_Form_Result<u32, Parse_Error>);
static_assert(Can_Form_Result<cstring, bool>);
static_assert(Can_Form_Result<void, int>);
static_assert(!Can_Form_Result<u32, cstring>);
static_assert(!Can_Form_Result<u32, float>);
static_assert(!Can_Form_Result<void, Not_An_Error>);

static Result<u32, Parse_Error> parse_digit(char c){
	if(c == 0){ return result_err(Parse_Error::Empty); }
	if(c < '0' || c > '9'){ return result_err(Parse_Error::Not_A_Digit); }
	return result_ok(u32(c - '0'));
}

static Result<u32, Parse_Error> parse_two_digits(cstring s){
	auto [hi, hi_err] = parse_digit(s[0]);
	or_return(hi_err);
	auto [lo, lo_err] = parse_digit(s[1]);
	or_return(lo_err);
	return result_ok(hi * 10 + lo);
}

static Result<void, Parse_Error> validate_digits(cstring s){
	or_return(parse_two_digits(s).error);
	return result_ok();
}

static int side_effects = 0;
static Result<u32, Parse_Error> counted_digit(char c){
	side_effects += 1;
	return parse_digit(c);
}

static Result<void, Parse_Error> validate_counted(char c){
	or_return(counted_digit(c).error);
	return result_ok();
}

// Chaining bare errors
static Result<void, Parse_Error> validate_all(cstring a, cstring b){
	or_return(validate_digits(a));
	or_return(validate_digits(b));
	return result_ok();
}

static Result<u32, int> parse_or_message(cstring s){
	auto [n, err] = parse_two_digits(s);
	if(is_error(err)){ return result_err(int(err)); }
	return result_ok(n);
}

// Propagating into a Result with a different value type
static Result<u64, Parse_Error> sum_pairs(cstring s){
	u64 total = 0;
	for(; s[0] != 0; s += 2){
		auto [n, err] = parse_two_digits(s);
		or_return(err);
		total += n;
	}
	return result_ok(total);
}

// Propagating a differently typed error that converts to the enclosing one
static Result<u64, int> sum_pairs_code(cstring s){
	auto [sum, err] = sum_pairs(s);
	if(is_error(err)){ return result_err(int(err)); }
	return result_ok(sum);
}

// `bool` errors are inverted, `true` means success
static Result<u32, bool> find_even(u32 x){
	if(x % 2 != 0){ return result_err(false); }
	return result_ok(x);
}

static Result<u32, bool> find_even_doubled(u32 x){
	auto [even, ok] = find_even(x);
	or_return(ok);
	return result_ok(even * 2);
}

static Result<void, bool> check_even(u32 x){
	or_return(find_even(x).error);
	return result_ok();
}

bool result_tests(){
	bool ok = true;
	ok &= test("error: integers", [](Test& t){
		check(!is_error(0));
		check(is_error(1));
		check(is_error(-1));
		check(is_error(i64(-1) << 40));
		check(!is_error(u8(0)) && is_error(u8(255)));
		check(!is_error(u64(0)) && is_error(~u64(0)));
		check(!is_error(i32(0)) && is_error(i32(min_val<i32>)));

		int const zero = 0;
		check(!is_error(zero));
		constexpr bool constant = is_error(5);
		check(constant);
	});

	ok &= test("error: bool", [](Test& t){
		// Inverted compared to everything else, `false` is the failure
		check(is_error(false));
		check(!is_error(true));
	});

	ok &= test("error: enums", [](Test& t){
		check(!is_error(Parse_Error::None));
		check(is_error(Parse_Error::Empty));
		check(is_error(Parse_Error::Not_A_Digit));

		check(!is_error(Plain_None));
		check(is_error(Plain_Bad));
		check(is_error(Plain_Negative));

		check(!is_error(IO_Error_None));
		check(is_error(IO_Error_EOF));
		check(is_error(IO_Error_Other));
	});

	ok &= test("error: is_error method", [](Test& t){
		check(!is_error(Own_Error{0}));
		check(!is_error(Own_Error{99}));
		check(is_error(Own_Error{100}));

		Syntax_Error none = {};
		check(!is_error(none));
		Syntax_Error bad = {};
		bad.type = CompErr_UnexpectedChar;
		check(is_error(bad));
		check(is_error(bad) == bad.is_error());
	});

	ok &= test("result: zero value", [](Test& t){
		Result<u32, Parse_Error> r = {};
		check(r.value == 0 && r.error == Parse_Error::None);
		check(!is_error(r.error));

		Result<u64, int> s = {};
		check(s.value == 0 && s.error == 0);

		Result<void, Parse_Error> v = {};
		check(!is_error(v));
	});

	ok &= test("result: value and error", [](Test& t){
		auto [a, a_err] = parse_digit('7');
		check(!is_error(a_err) && a == 7);

		// Errors come with the default value
		auto [b, b_err] = parse_digit('x');
		check(is_error(b_err) && b_err == Parse_Error::Not_A_Digit && b == 0);

		auto [c, c_err] = parse_digit(0);
		check(is_error(c_err) && c_err == Parse_Error::Empty);

		// Plain fields work as well
		Result<u32, Parse_Error> r = parse_digit('3');
		check(r.value == 3 && r.error == Parse_Error::None);

		// An error type different from the one propagated
		auto [m, m_err] = parse_or_message("a1");
		check(is_error(m_err) && m_err == int(Parse_Error::Not_A_Digit) && m == 0);
		check(parse_or_message("12").value == 12);
		check(!is_error(parse_or_message("12").error));
	});

	ok &= test("result: conversions", [](Test& t){
		// Different, but convertible, value and error types
		Result<u64, Parse_Error> wide = result_ok(u32(9));
		check(wide.value == 9 && !is_error(wide.error));

		Result<u32, int> coded = result_err(Plain_Bad);
		check(coded.value == 0 && coded.error == 7);

		Result<cstring, Parse_Error> str = result_ok("text");
		check(str.value != nullptr && !is_error(str.error));

		// Result<void, E> is E
		Parse_Error good = result_ok();
		check(!is_error(good));
		Parse_Error bad = result_err(Parse_Error::Empty);
		check(bad == Parse_Error::Empty);

		// `bool` errors succeed with `true`
		Result<u32, bool> yes = result_ok(4u);
		check(yes.value == 4 && !is_error(yes.error) && yes.error);
		Result<u32, bool> no = result_err(false);
		check(no.value == 0 && is_error(no.error));
		bool b_ok = result_ok();
		check(b_ok);
	});

	ok &= test("result: or_return", [](Test& t){
		auto [r, r_err] = parse_two_digits("42");
		check(!is_error(r_err) && r == 42);

		auto [hi_bad, hi_bad_err] = parse_two_digits("x2");
		check(is_error(hi_bad_err) && hi_bad_err == Parse_Error::Not_A_Digit && hi_bad == 0);

		auto [lo_bad, lo_bad_err] = parse_two_digits("4");
		check(is_error(lo_bad_err) && lo_bad_err == Parse_Error::Empty && lo_bad == 0);

		// Bare errors
		check(!is_error(validate_digits("99")));
		check(validate_digits("9?") == Parse_Error::Not_A_Digit);
		check(!is_error(validate_all("12", "34")));
		check(validate_all("12", "3x") == Parse_Error::Not_A_Digit);
		check(validate_all("x2", "34") == Parse_Error::Not_A_Digit);

		// Argument is evaluated exactly once
		side_effects = 0;
		check(!is_error(validate_counted('3')));
		check(is_error(validate_counted('z')));
		check(side_effects == 2);

		// Different value type
		check(sum_pairs("102030").value == 60);
		check(!is_error(sum_pairs("102030").error));
		auto [sum, sum_err] = sum_pairs("1020x0");
		check(is_error(sum_err) && sum_err == Parse_Error::Not_A_Digit && sum == 0);

		// Converting the error
		check(sum_pairs_code("1020").value == 30);
		check(sum_pairs_code("1x").error == int(Parse_Error::Not_A_Digit));
	});

	ok &= test("result: or_return with bool errors", [](Test& t){
		auto [d, d_ok] = find_even_doubled(4);
		check(!is_error(d_ok) && d == 8);

		auto [odd, odd_ok] = find_even_doubled(3);
		check(is_error(odd_ok) && odd == 0);

		check(!is_error(check_even(2)));
		check(is_error(check_even(5)));
	});
	return ok;
}

}
