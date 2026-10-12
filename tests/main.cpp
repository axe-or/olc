#include "arena.cpp"
#include "heap.cpp"
#include "slice.cpp"
#include "string.cpp"
#include "rune.cpp"
#include "dyn_array.cpp"
#include "map.cpp"
#include "result.cpp"

int main(){
	bool ok = true;
	ok &= test::arena_tests();
	ok &= test::heap_tests();
	ok &= test::slice_tests();
	ok &= test::string_tests();
	ok &= test::rune_tests();
	ok &= test::dyn_array_tests();
	ok &= test::map_tests();
	ok &= test::result_tests();

	if(!ok){
		printf("!!!! some tests failed !!!!\n");
		return 1;
	}

	return 0;
}

#include "../base.cpp"
