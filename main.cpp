#include "base.hpp"
#include "lexer.hpp"
#include <cstdio>

extern "C" int printf(char const*, ...);

// Print every token of `source`, stopping at the first error
void dump_tokens(String filepath, String source){
	Lexer lexer = { .filepath = filepath, .source = source, .current = 0 };
	for(;;){
		auto res = lexer.next();
		if(!res.has_value()){
			Syntax_Error err = res.unwrap_err();
			printf("%.*s:%d: error %d at offset %zu (got U+%04X)\n",
				(int)filepath.len(), filepath.raw_data(), err.line, err.type, err.offset, (u32)err.got.codepoint);
			return;
		}

		Token token = res.unwrap();
		String name = token_type_name(token.type);
		String text = source.slice(token.start, token.start + token.length);
		printf("%-14.*s %5zu %3u  %.*s\n",
			(int)name.len(), name.raw_data(), token.start, token.length, (int)text.len(), text.raw_data());
		if(token.type == Tk_EndOfFile){ return; }
	}
}

void entrypoint(Slice<String> args, Allocator heap, Allocator temp){
	for(usize i = 0; i < args.len(); i ++){
		printf("%.*s\n", STRF(args[i]));
	}
}

int main(int argc, char* argv[]){
	// Init heap
	constexpr usize heap_size = 8ull * 1024ull * 1024ull;
	u8 static heap_data[heap_size] = {0};
	auto heap = heap_from_buffer({&heap_data[0], heap_size});

	// Init temp
	constexpr usize temp_size = 1ull * 1024ull * 1024ull;
	u8 static temp_data[temp_size] = {0};
	auto temp = arena_from_buffer({&temp_data[0], temp_size});

	constexpr usize max_arg_count = 512;
	static String args_buf[max_arg_count] = {};

	ensure(usize(argc) < max_arg_count, "too many arguments");
	for(int i = 0; i < argc; i++){
		args_buf[i] = argv[i];
	}

	entrypoint(Slice<String>{&args_buf[0], usize(argc)}, heap.allocator(), temp.allocator());
	return 0;
}

#include "base.cpp"
#include "lexer.cpp"
