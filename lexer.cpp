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

static constexpr
bool is_alpha(u8 c){
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static constexpr
bool is_digit(u8 c){
	return c >= '0' && c <= '9';
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

// Identifier or keyword, made of ASCII letters, digits, `_` and any non-ASCII codepoint
static Result<Token, Syntax_Error> lex_identifier(Lexer* lx){
	usize start = lx->current;

	while(lx->current < lx->source.len()){
		u8 c = lx->source[lx->current];
		if(is_alpha(c) || is_digit(c)){
			lx->current += 1;
			continue;
		}
		if(c < 0x80){
			break;
		}

		Rune_Decoded r = decode_rune(lx);
		if(r.codepoint == RUNE_ERROR && r.size == 1){
			return result_err(make_error(lx, CompErr_Invalid_UTF8, lx->current));
		}
		lx->current += r.size;
	}

	Token tk = make_token(lx, start, Tk_Identifier);
	String text = lx->token_text(tk);

	#define X(sym, str) if(text == String(str)){ tk.type = Tk_##sym; }
	TOKENS_KEYWORDS
	#undef X

	return result_ok(tk);
}

// Operators and punctuation, always taking the longest match
static Result<Token, Syntax_Error> lex_separator(Lexer* lx){
	usize start = lx->current;
	u8 c = peek_byte(lx);
	u8 n = peek_byte(lx, 1);

	Token_Type type = Tk_Unknown;
	usize len = 1;

	switch(c){
		case '(': type = Tk_ParenOpen; break;
		case ')': type = Tk_ParenClose; break;
		case '[': type = Tk_SquareOpen; break;
		case ']': type = Tk_SquareClose; break;
		case '{': type = Tk_CurlyOpen; break;
		case '}': type = Tk_CurlyClose; break;
		case ':': type = Tk_Colon; break;
		case ';': type = Tk_Semicolon; break;
		case ',': type = Tk_Comma; break;
		case '.': type = Tk_Dot; break;
		case '+': type = Tk_Plus; break;
		case '*': type = Tk_Star; break;
		case '/': type = Tk_Slash; break;
		case '&': type = Tk_And; break;
		case '|': type = Tk_Or; break;
		case '~': type = Tk_Tilde; break;

		case '-':
			if(n == '>'){ type = Tk_Arrow; len = 2; }
			else { type = Tk_Minus; }
			break;

		case '<':
			if(n == '<'){ type = Tk_ShLeft; len = 2; }
			else if(n == '='){ type = Tk_LtEq; len = 2; }
			else { type = Tk_Lt; }
			break;

		case '>':
			if(n == '>'){ type = Tk_ShRight; len = 2; }
			else if(n == '='){ type = Tk_GtEq; len = 2; }
			else { type = Tk_Gt; }
			break;

		case '=':
			if(n == '='){ type = Tk_Eq; len = 2; }
			else { type = Tk_Assign; }
			break;

		case '!':
			if(n == '='){ type = Tk_Neq; len = 2; }
			break;
	}

	if(type == Tk_Unknown){
		Syntax_Error err = make_error(lx, CompErr_Unexpected_Char, start);
		err.got.codepoint = decode_rune(lx).codepoint;
		return result_err(err);
	}

	lx->current += len;
	return result_ok(make_token(lx, start, type));
}

Result<Token, Syntax_Error> Lexer::next(){
	skip_trivia(this);

	usize start = current;
	if(current >= source.len()){
		return result_ok(make_token(this, start, Tk_EndOfFile));
	}

	u8 c = source[current];
	if(is_alpha(c) || c >= 0x80){
		return lex_identifier(this);
	}

	return lex_separator(this);
}
