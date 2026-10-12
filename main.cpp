#include "base.hpp"

extern "C" int printf(char const*, ...);

struct Lexer {
	String source;
	usize current;
};

Allocator heap_allocator;

Allocator temp_allocator;

void entrypoint(){
}

void init(){
	// Init heap
	constexpr usize heap_size = 16ull * 1024ull * 1024ull;
	u8 static heap_data[heap_size] = {0};
	auto heap = heap_from_buffer({&heap_data[0], heap_size});
	heap_allocator = heap.allocator();

	// Init temp
	constexpr usize temp_size = 1ull * 1024ull * 1024ull;
	u8 static temp_data[temp_size] = {0};
	auto temp = arena_from_buffer({&temp_data[0], temp_size});
	temp_allocator = temp.allocator();

}

int main(){
	init();
	entrypoint();
}

#include "base.cpp"
