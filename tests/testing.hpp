#pragma once
#include <source_location>
#include <stdio.h>

#define CALLER_LOC std::source_location loc = std::source_location::current()

namespace test {

struct Test;

template<typename F>
concept TestFunc = requires (F f, Test& t) {
	f(t);
};

struct Test {
	char const* name = nullptr;
	int total = 0;
	int fail = 0;

	bool test(bool predicate, char const* msg, CALLER_LOC){
		total += 1;
		if(!predicate){
			fail += 1;
			fprintf(stderr, "test fail (%s:%d): %s\n", loc.file_name(), loc.line(), msg);
		}
		return predicate;
	}
};

// Shorthand for `t.test(expr, "expr")`, expects a `Test& t` in scope
#define check(expr) t.test((expr), #expr)

template<TestFunc F>
bool test(char const* name, F&& f){
	Test t = {.name = name};
	f(t);
	printf("[%s] %s ok in %d/%d\n", t.name, t.fail ? "FAIL" : "PASS", t.total - t.fail, t.total);
	return t.fail == 0;
}

}

#undef CALLER_LOC
