#include "lexer.hpp"

Lexer lexer_make(String filepath, String source){
	return Lexer{
		.filepath = filepath,
		.source = source,
		.current = 0,
		.line = 1,
	};
}

String Lexer::token_text(Token t){
	return source.slice(t.start, t.start + t.length);
}

// Byte `ahead` positions past the current one, 0 when out of bounds
static u8 peek_byte(Lexer* lx, usize ahead = 0){
	usize pos = lx->current + ahead;
	return pos < lx->source.len() ? lx->source[pos] : 0;
}

// Decode the rune at the current position
static Rune_Decoded decode_rune(Lexer* lx){
	usize remaining = lx->source.len() - lx->current;
	return rune_decode((u8 const*)lx->source.raw_data() + lx->current, u32(min(remaining, usize(4))));
}

// Token spanning from `start` to the current position
static Token make_token(Lexer* lx, usize start, Token_Type type){
	return Token{
		.start = start,
		.length = u32(lx->current - start),
		.type = type,
	};
}

static Syntax_Error make_error(Lexer* lx, Compiler_Error_Type type, usize offset){
	Syntax_Error err = {};
	err.filepath = lx->filepath;
	err.offset = offset;
	err.line = lx->line;
	err.type = type;
	return err;
}

// Skip whitespace and line comments, counting lines
static void skip_trivia(Lexer* lx){
	while(lx->current < lx->source.len()){
		u8 c = lx->source[lx->current];
		if(c == '\n'){
			lx->line += 1;
			lx->current += 1;
		}
		else if(c == ' ' || c == '\t' || c == '\r'){
			lx->current += 1;
		}
		else if(c == '/' && peek_byte(lx, 1) == '/'){
			// Stop at the newline so it gets counted
			while(lx->current < lx->source.len() && lx->source[lx->current] != '\n'){
				lx->current += 1;
			}
		}
		else {
			break;
		}
	}
}

Result<Token, Syntax_Error> Lexer::next(){
	skip_trivia(this);

	usize start = current;
	if(current >= source.len()){
		return result_ok(make_token(this, start, Tk_EndOfFile));
	}

	Syntax_Error err = make_error(this, CompErr_Unexpected_Char, start);
	err.got.codepoint = decode_rune(this).codepoint;
	return result_err(err);
}
