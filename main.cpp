#include "base.hpp"
#include "lexer.hpp"

extern "C" int printf(char const*, ...);

void entrypoint(){
}

void init(){
	// Init heap
	constexpr usize heap_size = 16ull * 1024ull * 1024ull;
	u8 static heap_data[heap_size] = {0};
	auto heap = heap_from_buffer({&heap_data[0], heap_size});

	// Init temp
	constexpr usize temp_size = 1ull * 1024ull * 1024ull;
	u8 static temp_data[temp_size] = {0};
	auto temp = arena_from_buffer({&temp_data[0], temp_size});
}

int main(){
	init();
	entrypoint();
}

#include "lexer.cpp"
#include "base.cpp"
