#include "base.hpp"

extern "C" int printf(char const*, ...);

void entrypoint(){
}

void init(){
	u8 static heap_data[16ull * 1024ull * 1024ull] = {0};
	auto heap = heap_from_buffer({&heap_data[0], sizeof(heap_data)});
}

int main(){
	init();
	entrypoint();
}

#include "base.cpp"
