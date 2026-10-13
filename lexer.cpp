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

// Value of `c` as a hexadecimal digit, or -1 if it is not one
static constexpr
i32 hex_digit_value(u8 c){
	if(c >= '0' && c <= '9'){ return c - '0'; }
	if(c >= 'a' && c <= 'f'){ return c - 'a' + 10; }
	if(c >= 'A' && c <= 'F'){ return c - 'A' + 10; }
	return -1;
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

// Consume a run of digits in `base` with `_` separators, the run must start and end with a digit
static bool scan_digits(Lexer* lx, i32 base){
	usize start = lx->current;
	for(;;){
		u8 c = peek_byte(lx);
		i32 v = hex_digit_value(c);
		if(v >= 0 && v < base){
			lx->current += 1;
		}
		else if(c == '_' && lx->current > start){
			lx->current += 1;
		}
		else {
			break;
		}
	}
	return lx->current > start && lx->source[lx->current - 1] != '_';
}

// Integer in base 10, 16 (0x), 2 (0b) or 8 (0o), or a decimal Real with a fraction and/or exponent
static Result<Token, Syntax_Error> lex_number(Lexer* lx){
	usize start = lx->current;
	Token_Type type = Tk_Integer;

	i32 base = 10;
	if(peek_byte(lx) == '0'){
		switch(peek_byte(lx, 1)){
			case 'x': base = 16; break;
			case 'b': base = 2; break;
			case 'o': base = 8; break;
		}
	}

	bool ok = true;
	if(base != 10){
		lx->current += 2;
		ok = scan_digits(lx, base);
	}
	else {
		ok = scan_digits(lx, 10);

		// No trailing `.`, a fraction needs digits
		if(ok && peek_byte(lx) == '.'){
			lx->current += 1;
			type = Tk_Real;
			ok = scan_digits(lx, 10);
		}

		if(ok && (peek_byte(lx) == 'e' || peek_byte(lx) == 'E')){
			lx->current += 1;
			type = Tk_Real;
			if(peek_byte(lx) == '+' || peek_byte(lx) == '-'){
				lx->current += 1;
			}
			ok = scan_digits(lx, 10);
		}
	}

	// Reject identifier characters glued to the literal, e.g. `123abc` or `0b102`
	u8 c = peek_byte(lx);
	if(is_alpha(c) || is_digit(c) || c >= 0x80){
		ok = false;
	}

	if(!ok){
		return result_err(make_error(lx, CompErr_Malformed_Number, start));
	}
	return result_ok(make_token(lx, start, type));
}

// Validate an escape sequence starting at the backslash: \n \r \t \\ \" or \u{X} with 1 to 6 hex digits
static Result<void, Syntax_Error> scan_escape(Lexer* lx){
	usize start = lx->current;
	lx->current += 1; // Backslash

	switch(peek_byte(lx)){
		case 'n': case 'r': case 't': case '\\': case '"':
			lx->current += 1;
			return result_ok();

		case 'u':
			lx->current += 1;
			break;

		default: {
			Syntax_Error err = make_error(lx, CompErr_Invalid_Escape, start);
			err.got.codepoint = decode_rune(lx).codepoint;
			return result_err(err);
		}
	}

	if(peek_byte(lx) != '{'){
		return result_err(make_error(lx, CompErr_Invalid_Codepoint, start));
	}
	lx->current += 1;

	rune value = 0;
	i32 digits = 0;
	for(i32 v = hex_digit_value(peek_byte(lx)); v >= 0; v = hex_digit_value(peek_byte(lx))){
		if(digits < 6){
			value = value * 16 + v;
		}
		digits += 1;
		lx->current += 1;
	}

	bool is_surrogate = value >= 0xd800 && value <= 0xdfff;
	if(digits < 1 || digits > 6 || peek_byte(lx) != '}' || value > 0x10ffff || is_surrogate){
		return result_err(make_error(lx, CompErr_Invalid_Codepoint, start));
	}
	lx->current += 1;

	return result_ok();
}

// Single line, double quoted string. The token spans the quotes, escapes are validated but not decoded
static Result<Token, Syntax_Error> lex_string(Lexer* lx){
	usize start = lx->current;
	lx->current += 1; // Opening quote

	for(;;){
		if(lx->current >= lx->source.len() || peek_byte(lx) == '\n'){
			return result_err(make_error(lx, CompErr_Unterminated_String, start));
		}

		u8 c = peek_byte(lx);
		if(c == '"'){
			lx->current += 1;
			break;
		}

		if(c == '\\'){
			auto esc = scan_escape(lx);
			if(!esc.has_value()){
				return result_err(esc.unwrap_err());
			}
		}
		else if(c >= 0x80){
			Rune_Decoded r = decode_rune(lx);
			if(r.codepoint == RUNE_ERROR && r.size == 1){
				return result_err(make_error(lx, CompErr_Invalid_UTF8, lx->current));
			}
			lx->current += r.size;
		}
		else {
			lx->current += 1;
		}
	}

	return result_ok(make_token(lx, start, Tk_String));
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
	if(is_digit(c)){
		return lex_number(this);
	}
	if(c == '"'){
		return lex_string(this);
	}

	return lex_separator(this);
}

Result<Token, Syntax_Error> Lexer::peek(){
	Lexer tmp = *this;
	return tmp.next();
}
