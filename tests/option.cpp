#include "testing.hpp"
#include "../base.hpp"

namespace test {

static Option<i32> half(i32 x){
	if(x % 2 != 0){ return None; }
	return Some(x / 2);
}

static_assert(Option<i32>{}.is_none());
static_assert(Some(3).is_some() && Some(3).unwrap_or(0) == 3);

bool option_tests(){
	bool ok = true;
	ok &= test("option: construction and queries", [](Test& t){
		Option<i32> zero{};
		Option<i32> none = None;
		Option<i32> some = Some(10);
		Option<i32> implicit = 7;

		check(zero.is_none() && !zero.is_some());
		check(none.is_none());
		check(some.is_some() && !some.is_none());
		check(implicit.is_some() && implicit.unwrap() == 7);

		// The zero value has a zeroed payload
		check(zero.unwrap_unchecked() == 0);

		auto big = [](i32 x){ return x > 5; };
		check(some.is_some_and(big) && !Some(1).is_some_and(big) && !none.is_some_and(big));
		check(none.is_none_or(big) && some.is_none_or(big) && !Some(1).is_none_or(big));

		check(zero.as_ptr() == nullptr);
		check(some.as_ptr() != nullptr && *some.as_ptr() == 10);
		*some.as_ptr() = 11;
		check(some.unwrap() == 11);
	});

	ok &= test("option: extraction", [](Test& t){
		Option<i32> none{};
		Option<i32> some = Some(4);

		check(some.unwrap() == 4);
		check(some.expect("must have a value") == 4);
		check(some.unwrap_unchecked() == 4);
		check(some.unwrap_or(9) == 4 && none.unwrap_or(9) == 9);
		check(some.unwrap_or_else([]{ return 9; }) == 4);
		check(none.unwrap_or_else([]{ return 9; }) == 9);
		check(some.unwrap_or_default() == 4 && none.unwrap_or_default() == 0);
	});

	ok &= test("option: combinators", [](Test& t){
		Option<i32> none{};
		Option<i32> some = Some(10);

		Option<f64> mapped = some.map([](i32 x){ return x * 1.5; });
		check(mapped.is_some() && mapped.unwrap() == 15.0);
		check(none.map([](i32 x){ return x * 1.5; }).is_none());

		auto inc = [](i32 x){ return x + 1; };
		check(some.map_or(0, inc) == 11 && none.map_or(0, inc) == 0);
		check(some.map_or_else([]{ return -1; }, inc) == 11);
		check(none.map_or_else([]{ return -1; }, inc) == -1);

		check(some.and_then(half) == Some(5));
		check(Some(3).and_then(half).is_none());
		check(none.and_then(half).is_none());

		check(some.and_(Some(2.5)).unwrap() == 2.5);
		check(none.and_(Some(2.5)).is_none());

		check(some.or_(Some(1)) == Some(10));
		check(none.or_(Some(1)) == Some(1));
		check(none.or_(None).is_none());
		check(some.or_else([]{ return Some(1); }) == Some(10));
		check(none.or_else([]{ return Some(1); }) == Some(1));

		check(some.xor_(None) == Some(10));
		check(none.xor_(Some(1)) == Some(1));
		check(some.xor_(Some(1)).is_none());
		check(none.xor_(None).is_none());

		auto big = [](i32 x){ return x > 5; };
		check(some.filter(big) == Some(10));
		check(Some(1).filter(big).is_none());
		check(none.filter(big).is_none());
	});

	ok &= test("option: mutation", [](Test& t){
		Option<i32> o = Some(1);
		Option<i32> taken = o.take();
		check(taken == Some(1) && o.is_none() && o.unwrap_unchecked() == 0);
		check(o.take().is_none());

		o = Some(6);
		auto big = [](i32 x){ return x > 5; };
		check(o.take_if(big) == Some(6) && o.is_none());
		o = Some(2);
		check(o.take_if(big).is_none() && o == Some(2));

		check(o.replace(3) == Some(2) && o == Some(3));
		check(Option<i32>{}.replace(3).is_none());

		Option<i32> n{};
		n.insert(4) += 1;
		check(n == Some(5));

		Option<i32> g{};
		check(g.get_or_insert(7) == 7 && g.get_or_insert(8) == 7);
		Option<i32> w{};
		check(w.get_or_insert_with([]{ return 9; }) == 9 && w == Some(9));
		check(w.get_or_insert_with([]{ return 1; }) == 9);
		Option<i32> d{};
		check(d.get_or_insert_default() == 0 && d.is_some());
	});

	ok &= test("option: equality", [](Test& t){
		check(Some(1) == Some(1));
		check(!(Some(1) == Some(2)));
		check(!(Some(1) == Option<i32>{}));
		check(Option<i32>{} == Option<i32>{});
		check(Option<i32>{} == None && !(Some(0) == None));
		check(Some(String("abc")) == Some(String("abc")));
	});
	return ok;
}

}
