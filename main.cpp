#include "base.hpp"

extern "C" int printf(char const*, ...);

enum Token_Type : u8 {
	Tk_Unknown = 0,

	Tk_CurlyOpen,
};

enum Compiler_Error_Type : u16 {
	CompErr_None = 0,
};

// Error for the lexing and parsing phase
struct Syntax_Error {
	String filepath;
	usize offset;
	i32 line;
	Compiler_Error_Type type;

	union {
		rune codepoint;
		Token_Type tk_type;
	} expected;

	union {
		rune codepoint;
		Token_Type tk_type;
	} got;
};

struct Lexer {
	String source;
	usize current;
};

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

#include "base.cpp"
