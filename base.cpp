#include "base.hpp"
#include "lib/tlsf.h"
#include <cstddef>
#include <string.h>

//// Assertions
extern "C" {
	void abort();
	int printf(cstring, ...);
}

[[noreturn]]
void trap(){
	abort();
	while(1);
}

[[noreturn]] void panic(cstring msg, Source_Location loc){
	printf("(%s:%d) panic: %s\n", loc.file_name(), loc.line(), msg);
	trap();
}


bool ensure(bool pred, cstring msg, Source_Location loc){
	if(!pred){
		printf("(%s:%d) assertion failed: %s\n", loc.file_name(), loc.line(), msg);
		trap();
	}
	return pred;
}

//// String
#define MASKX 0x3f /* 0011_1111 */
#define MASK2 0x1f /* 0001_1111 */
#define MASK3 0x0f /* 0000_1111 */
#define MASK4 0x07 /* 0000_0111 */

#define CONT_LO 0x80
#define CONT_HI 0xbf

struct UTF8_Accept_Range { u8 lo, hi; };

static const
struct UTF8_Accept_Range utf8_accept_ranges[5] = {
	{0x80, 0xbf},
	{0xa0, 0xbf},
	{0x80, 0x9f},
	{0x90, 0xbf},
	{0x80, 0x8f},
};

static const u8 utf8_accept_sizes[256] = {
	0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,
	0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,
	0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,
	0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,
	0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,
	0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,
	0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,
	0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,
	0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,
	0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,
	0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,
	0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,
	0xf1,0xf1,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,
	0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,
	0x13,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x23,0x03,0x03,
	0x34,0x04,0x04,0x04,0x44,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,
};

Rune_Decoded rune_decode(u8 const* buf, u32 buflen){
	constexpr Rune_Decoded error = { .codepoint = RUNE_ERROR, .size = 1 };
	Rune_Decoded result = {};

	if(buflen < 1){
		return result;
	}

	u8 b0 = buf[0];
	u8 x = utf8_accept_sizes[b0];

	// ASCII or invalid
	if(x >= 0xf0){
		u32 mask = ((rune)(x) << 31) >> 31; // Either all 0's or all 1's to avoid branching
		result.codepoint = ((rune)(b0) & ~mask) | (RUNE_ERROR & mask);
		result.size = 1;
		return result;
	}

	u8 sz = x & 7;
	struct UTF8_Accept_Range accept = utf8_accept_ranges[x >> 4];

	if(buflen < sz){
		return error;
	}

	u8 b1 = buf[1];
	if(b1 < accept.lo || accept.hi < b1){
		return error;
	}
	if(sz == 2){
		result.codepoint = ((rune)(b0 & MASK2) << 6) | ((rune)(b1 & MASKX));
		result.size = 2;
		return result;
	}

	u8 b2 = buf[2];
	if(b2 < CONT_LO || CONT_HI < b2){
		return error;
	}

	if(sz == 3){
		result.codepoint = ((rune)(b0 & MASK3) << 12) | ((rune)(b1 & MASKX) << 6) | (rune)(b2 & MASKX);
		result.size = 3;
		return result;
	}

	u8 b3 = buf[3];
	if(b3 < CONT_LO || CONT_HI < b3){
		return error;
	}

	result.codepoint = ((rune)(b0 & MASK4) << 18) | ((rune)(b1 & MASKX) << 12) | ((rune)(b2 & MASKX) << 6) | (rune)(b3 & MASKX);
	result.size = 4;
	return result;
}

Rune_Encoded rune_encode(rune r){
	constexpr u8 mask = 0x3f;
	Rune_Encoded result = {};

	if(r <= 0x7f){ // 1-wide (ASCII)
		return (Rune_Encoded){ .bytes = {(u8)r}, .size = 1 };
	}

	if(r <= 0x7ff){ // 2-wide
		result.bytes[0] = 0xc0 |  (u8)(r >> 6);
		result.bytes[1] = 0x80 | ((u8)(r) & mask);
		result.size = 2;
		return result;
	}

	// Surrogate or invalid -> Encode the error rune
	if((r > 0x10ffff) || ((0xd800 <= r) && (r <= 0xdfff))){
		r = 0xfffd;
	}

	if(r <= 0xffff){ // 3-wide
		result.bytes[0] = 0xe0 |  (u8)(r >> 12);
		result.bytes[1] = 0x80 | ((u8)(r >> 6) & mask);
		result.bytes[2] = 0x80 | ((u8)(r)      & mask);
		result.size = 3;
		return result;
	}
	else { // 4-wide
		result.bytes[0] = 0xf0 |  (u8)(r >> 18);
		result.bytes[1] = 0x80 | ((u8)(r >> 12) & mask);
		result.bytes[2] = 0x80 | ((u8)(r >> 6)  & mask);
		result.bytes[3] = 0x80 | ((u8)(r)       & mask);
		result.size = 4;
		return result;
	}
}

//// Arena

Arena arena_from_buffer(void* buffer, usize size){
	ensure(buffer != nullptr || size == 0, "invalid arena buffer");
	return Arena{
		.data = (u8*)buffer,
		.capacity = size,
		.offset = 0,
		.last_allocation = nullptr,
		.last_allocation_size = 0,
	};
}

bool Arena::owns(void const* ptr) const{
	uintptr address = (uintptr)ptr;
	uintptr base = (uintptr)data;
	return ptr != nullptr && data != nullptr && address >= base && address - base < capacity;
}

void* Arena::alloc(usize size, usize align){
	if(size == 0){ return nullptr; }
	ensure(align != 0 && (align & (align - 1)) == 0, "invalid arena alignment");
	if(data == nullptr){ return nullptr; }

	uintptr current = (uintptr)(data + offset);
	usize padding = (0 - current) & (align - 1);
	usize available = capacity - offset;

	// Avoid wrap-around
	if(padding > available || size > (available - padding)){
		return nullptr;
	}

	void* allocation = data + offset + padding;
	offset += padding + size;
	memset(allocation, 0, size);
	last_allocation = allocation;
	last_allocation_size = size;
	return allocation;
}

bool Arena::resize(void* ptr, usize new_size){
	if(ptr == nullptr){ return false; }
	ensure(owns(ptr), "pointer not owned by arena");
	if(ptr != last_allocation){ return false; }

	usize start = (u8*)ptr - data;
	if(new_size > capacity - start){ return false; }
	if(new_size > last_allocation_size){
		memset((u8*)ptr + last_allocation_size, 0, new_size - last_allocation_size);
	}
	offset = start + new_size;
	last_allocation_size = new_size;
	return true;
}

void* Arena::realloc(void* ptr, usize old_size, usize new_size, usize align){
	if(ptr == nullptr){ return alloc(new_size, align); }
	ensure(align != 0 && (align & (align - 1)) == 0, "invalid arena alignment");
	if((uintptr)ptr % align == 0 && resize(ptr, new_size)){ return ptr; }

	void* allocation = alloc(new_size, align);
	if(allocation == nullptr){ return nullptr; }
	memcpy(allocation, ptr, min(old_size, new_size));
	return allocation;
}

void Arena::reset(){
	offset = 0;
	last_allocation = nullptr;
	last_allocation_size = 0;
}

static
uintptr arena_allocator_proc(void* impl, Allocator_Mode mode, void* ptr, Memory_Layout old, Memory_Layout desired){
	Arena* arena = static_cast<Arena*>(impl);
	switch(mode){
	case Mem_Query:
		return Mem_Alloc | Mem_Grow | Mem_Shrink | Mem_FreeAll;
	case Mem_Alloc:
		return (uintptr)arena->alloc(desired.size, desired.align);
	case Mem_Grow:
	case Mem_Shrink:
		return (uintptr)arena->realloc(ptr, old.size, desired.size, desired.align);
	case Mem_Free:
		// Individual allocations live until the arena is reset.
		return 0;
	case Mem_FreeAll:
		arena->reset();
		return 0;
	}
	panic("invalid allocator mode");
}

Allocator Arena::allocator(){
	return Allocator{ this, arena_allocator_proc };
}

//// Heap allocator
void* Heap_Allocator::alloc(Memory_Layout layout) {
	return tlsf_memalign(impl, layout.align, layout.size);
}

void Heap_Allocator::free(void* p) {
	return tlsf_free(impl, p);
}

void* Heap_Allocator::realloc(void* ptr, Memory_Layout old, Memory_Layout desired) {
	if(old.align == desired.align){
		void* p = tlsf_realloc(impl, ptr, desired.size);
		ensure((uintptr(p) & (desired.align - 1)) == 0, "invalid alignment after realloc");
		return p;
	}

	void* data = tlsf_memalign(impl, desired.align, desired.size);
	usize n = min(old.size, desired.size);
	if(data){
		mem_copy_no_overlap(data, ptr, n);
		tlsf_free(impl, ptr);
		if(old.size < desired.size){
			void* dest = (void*)(uintptr(data) + old.size);
			mem_zero(dest, desired.size - old.size);
		}
	}
	return data;
}

Heap_Allocator heap_from_buffer(Slice<u8> buf){
	ensure(tlsf_size() < buf.len(), "not enough space for tlsf metadata");
	auto impl = tlsf_create_with_pool(buf.raw_data(), buf.len());
	ensure(impl != NULL, "failed to create with pool");
	return Heap_Allocator{ impl };
}

static
uintptr heap_allocator_proc(void* impl, Allocator_Mode mode, void* ptr, Memory_Layout old, Memory_Layout desired) {
	auto heap = (Heap_Allocator*)impl;
	void* res = nullptr;

	switch (mode) {
		case Mem_Query:
			return Mem_Alloc | Mem_Grow | Mem_Shrink | Mem_Free;

		case Mem_Alloc:
			res = heap->alloc(desired);
			return (uintptr)res;

		case Mem_Grow:
			return (uintptr)heap->realloc(ptr, old, desired);

		case Mem_Shrink:
			return (uintptr)heap->realloc(ptr, old, desired);

		case Mem_Free:
			heap->free(ptr);
			return 0;

		case Mem_FreeAll:
			return 0; /* Unsupported */
	}

	panic("invalid allocator mode");
}

Allocator Heap_Allocator::allocator(){
	return {
		.impl_ = this,
		.proc_ = heap_allocator_proc,
	};
}
