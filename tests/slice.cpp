#include "testing.hpp"
#include "../base.hpp"

namespace test {

void slice_tests(){
	test("slice: basics", [](Test& t){
		Slice<i32> empty;
		check(empty.len() == 0 && empty.raw_data() == nullptr);

		i32 nums[] = {10, 20, 30, 40, 50};
		Slice<i32> s{nums, 5};
		check(s.len() == 5 && s.raw_data() == nums);
		check(s[0] == 10 && s[4] == 50);

		s[2] = 33;
		check(nums[2] == 33);

		Slice<i32> const cs = s;
		check(cs[2] == 33);
	});

	test("slice: take, skip, slice", [](Test& t){
		i32 nums[] = {10, 20, 30, 40, 50};
		Slice<i32> s{nums, 5};

		Slice<i32> front = s.take(2);
		check(front.len() == 2 && front[0] == 10 && front[1] == 20);
		check(s.take(0).len() == 0);
		check(s.take(5).len() == 5);

		Slice<i32> back = s.skip(3);
		check(back.len() == 2 && back[0] == 40 && back[1] == 50);
		check(s.skip(5).len() == 0);

		Slice<i32> mid = s.slice(1, 4);
		check(mid.len() == 3 && mid[0] == 20 && mid[2] == 40);
		check(s.slice(2, 2).len() == 0);
		check(s.slice(0, 5).len() == 5);

		// Views alias the original storage
		mid[0] = 21;
		check(nums[1] == 21);
	});

	test("slice: copy", [](Test& t){
		i32 src_data[] = {1, 2, 3, 4};
		i32 dst_data[] = {0, 0, 0, 0, 0, 0};

		Slice<i32> copied = copy(Slice<i32>{dst_data, 6}, Slice<i32>{src_data, 4});
		check(copied.len() == 4 && copied.raw_data() == dst_data);
		check(dst_data[0] == 1 && dst_data[3] == 4 && dst_data[4] == 0);

		i32 small[] = {9, 9};
		copied = copy(Slice<i32>{small, 2}, Slice<i32>{src_data, 4});
		check(copied.len() == 2 && small[0] == 1 && small[1] == 2);

		copied = copy(Slice<i32>{}, Slice<i32>{src_data, 4});
		check(copied.len() == 0);
	});
}

}
