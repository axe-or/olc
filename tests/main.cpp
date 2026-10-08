#include "../base.hpp"
#include "testing.hpp"

#include "arena.cpp"
#include "heap.cpp"
#include "slice.cpp"
#include "string.cpp"
#include "rune.cpp"
#include "dyn_array.cpp"
#include "map.cpp"
#include "option.cpp"

int main(){
	test::arena_tests();
	test::heap_tests();
	test::slice_tests();
	test::string_tests();
	test::rune_tests();
	test::dyn_array_tests();
	test::map_tests();
	test::option_tests();

	if(test::failed_groups != 0){
		printf("%d test group(s) failed\n", test::failed_groups);
		return 1;
	}
	return 0;
}

#include "../base.cpp"
