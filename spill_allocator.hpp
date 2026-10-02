#include "base.hpp"

// Allocator that tries to optimistically allocate with `first` and if not possible it spills the
// request to the `fallback` allocator. This allocator is most useful when the first allocator is
// very fast for frequent range of allocation but pathological cases where a robust fallback is required.
//
// An allocation may move between `first` and `fallback` whenever it is resized. Both allocators
// must be valid at once.
struct Spill_Allocator {
	Allocator first;
	Allocator fallback;

	Allocator allocator();
};

static inline
uintptr spill_allocator_proc(){
	todo();
}
