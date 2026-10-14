#pragma once

#include "base.hpp"

#define TOKENS_OP \
	X(ParenOpen, "(") \
	X(ParenClose, ")") \
	X(SquareOpen, "[") \
	X(SquareClose, "]") \
	X(CurlyOpen, "{") \
	X(CurlyClose, "}") \
	X(Dot, ".") \
	X(Comma, ",") \
	X(Semicolon, ";") \
	X(Colon, ":") \
	X(Question, "?") \
	X(Caret, "^") \
	X(Plus, "+") \
	X(Minus, "-") \
	X(Star, "*") \
	X(Slash, "/") \
	X(Mod, "%") \
	X(ModFloor, "%%") \
	X(BitAnd, "&") \
	X(BitOr, "|") \
	X(Tilde, "~") \
	X(ShiftLeft, "<<") \
	X(ShiftRight, ">>") \
	X(PlusAssign, "+=") \
	X(MinusAssign, "-=") \
	X(StarAssign, "*=") \
	X(SlashAssign, "/=") \
	X(ModAssign, "%=") \
	X(ModFloorAssign, "%%=") \
	X(BitAndAssign, "&=") \
	X(BitOrAssign, "|=") \
	X(TildeAssign, "~=") \
	X(ShiftLeftAssign, "<<=") \
	X(ShiftRightAssign, ">>=") \
	X(Assign, "=") \
	X(Equal, "==") \
	X(NotEqual, "!=") \
	X(Less, "<") \
	X(Greater, ">") \
	X(LessEqual, "<=") \
	X(GreaterEqual, ">=") \
	X(Arrow, "->") \

#define TOKENS_KEYWORD \
	X(Var, "var") \
	X(Proc, "proc") \
	X(If, "if") \
	X(And, "and") \
	X(Or, "or") \
	X(Not, "not")

#define TOKENS \
	TOKENS_OP \
	TOKENS_KEYWORD


enum Token_Type : u8 {
	Tk_Unknown = 0,

	Tk_Identifier,
	Tk_Integer,
	Tk_Float,
	Tk_String,
	Tk_Char,
	Tk_EndOfFile,

	#define X(sym, str) Tk_##sym,
	TOKENS
	#undef X
};

constexpr
String token_type_name(Token_Type t){
	switch(t){
		case Tk_Unknown: return "<Unknown>";
		case Tk_Identifier: return "<Identifier>";
		case Tk_Integer: return "<Integer>";
		case Tk_Float: return "<Float>";
		case Tk_String: return "<String>";
		case Tk_Char: return "<Char>";
		case Tk_EndOfFile: return "<EOF>";
		#define X(sym, str) case Tk_##sym : return str;
		TOKENS
		#undef X
		default: panic("invalid token type");
	}
}

struct Token {
	usize start;
	u32 length;
	Token_Type type;
};

enum Compiler_Error_Type : u16 {
	CompErr_None = 0,

	CompErr_UnexpectedChar,
	CompErr_InvalidUtf8,
	CompErr_UnterminatedString,
	CompErr_UnterminatedChar,
	CompErr_UnterminatedComment,
	CompErr_InvalidEscape,
	CompErr_InvalidCodepoint,
	CompErr_MalformedNumber,
	CompErr_EmptyChar,
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

static inline
bool is_digit(u8 c){
	return c >= '0' && c <= '9';
}

static inline
bool is_hex_digit(u8 c){
	return is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static inline
bool is_alpha(u8 c){
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static inline
bool is_ident_char(u8 c){
	return is_alpha(c) || is_digit(c) || c == '_';
}

static inline
bool is_whitespace(u8 c){
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static inline
rune hex_value(u8 c){
	if(is_digit(c)){ return c - '0'; }
	if(c >= 'a' && c <= 'f'){ return c - 'a' + 10; }
	return c - 'A' + 10;
}

struct Lexer {
	String filepath;
	String source;
	usize current;

	// Get the next token and advance
	Result<Token, Syntax_Error> next(){
		err_return(skip_whitespace_and_comments());

		usize start = current;
		if(current >= source.len()){
			return result_ok(Token{start, 0, Tk_EndOfFile});
		}

		u8 c = peek_char();
		if(is_alpha(c) || c == '_'){
			consume_while(is_ident_char);
			String text = source.slice(start, current);
			Token_Type type = Tk_Identifier;
			#define X(sym, str) if(text == String(str)){ type = Tk_##sym; }
			TOKENS_KEYWORD
			#undef X
			return result_ok(make_token(start, type));
		}
		if(is_digit(c)){ return lex_number(); }
		if(c == '"'){ return lex_string(); }
		if(c == '\''){ return lex_char(); }
		return lex_operator();
	}

	// Get the next token without advancing
	Result<Token, Syntax_Error> peek(){
		usize saved = current;
		auto token = next();
		current = saved;
		return token;
	}

	// Byte `ahead` positions from the current one, 0 if past the end
	u8 peek_char(usize ahead = 0){
		usize pos = current + ahead;
		return pos < source.len() ? source[pos] : 0;
	}

	// Source at the current position starts with `s`
	bool source_prefixed_with(String s){
		if(current + s.len() > source.len()){ return false; }
		return source.slice(current, current + s.len()) == s;
	}

	template<Predicate<u8> F>
	bool consume_if(F pred){
		if(current < source.len() && pred(source[current])){
			current += 1;
			return true;
		}
		return false;
	}

	bool consume_char(u8 c){
		return consume_if([c](u8 x){ return x == c; });
	}

	// Returns how many bytes were consumed
	template<Predicate<u8> F>
	usize consume_while(F pred){
		usize start = current;
		while(consume_if(pred)){}
		return current - start;
	}

	// Consume digits and `_` separators, returns how many actual digits were consumed
	template<Predicate<u8> F>
	usize consume_digits(F pred){
		usize n = 0;
		for(;;){
			if(consume_if(pred)){ n += 1; }
			else if(!consume_char('_')){ break; }
		}
		return n;
	}

	// Consume a single UTF-8 encoded codepoint
	Result<void, Syntax_Error> consume_rune(){
		String rest = source.skip(current);
		Rune_Decoded r = rune_decode((u8 const*)rest.raw_data(), (u32)min(rest.len(), usize(4)));
		if(r.codepoint == RUNE_ERROR && r.size == 1){
			return result_err(error_at(current, CompErr_InvalidUtf8, rest[0]));
		}
		current += r.size;
		return result_ok();
	}

	Token make_token(usize start, Token_Type type){
		return Token{start, u32(current - start), type};
	}

	Syntax_Error error_at(usize offset, Compiler_Error_Type type, rune got = 0){
		i32 line = 1;
		for(usize i = 0; i < offset && i < source.len(); i += 1){
			if(source[i] == '\n'){
				line += 1;
			}
		}

		Syntax_Error err = {
			.filepath = filepath,
			.offset = offset,
			.line = line,
			.type = type,
			.expected = {0},
			.got = { .codepoint = got },
		};
		return err;
	}

	Result<void, Syntax_Error> skip_whitespace_and_comments(){
		for(;;){
			consume_while(is_whitespace);

			if(source_prefixed_with("//")){
				consume_while([](u8 c){ return c != '\n'; });
			}
			else if(source_prefixed_with("/*")){
				usize start = current;
				i32 depth = 0;
				do {
					if(current >= source.len()){
						return result_err(error_at(start, CompErr_UnterminatedComment));
					}
					if(source_prefixed_with("/*")){
						depth += 1;
						current += 2;
					}
					else if(source_prefixed_with("*/")){
						depth -= 1;
						current += 2;
					}
					else {
						current += 1;
					}
				} while(depth > 0);
			}
			else {
				return result_ok();
			}
		}
	}

	// Longest operator matching the current position
	Result<Token, Syntax_Error> lex_operator(){
		usize start = current;
		Token_Type type = Tk_Unknown;
		usize length = 0;

		#define X(sym, str) if(String(str).len() > length && source_prefixed_with(str)){ type = Tk_##sym; length = String(str).len(); }
		TOKENS_OP
		#undef X

		if(type == Tk_Unknown){
			String rest = source.skip(current);
			Rune_Decoded r = rune_decode((u8 const*)rest.raw_data(), (u32)min(rest.len(), usize(4)));
			return result_err(error_at(start, CompErr_UnexpectedChar, r.codepoint));
		}

		current += length;
		return result_ok(make_token(start, type));
	}

	Result<Token, Syntax_Error> lex_number(){
		usize start = current;
		Token_Type type = Tk_Integer;
		u8 prefix = peek_char(1);

		if(peek_char() == '0' && (prefix == 'b' || prefix == 'o' || prefix == 'x')){
			current += 2;
			usize digits = 0;
			switch(prefix){
				case 'b': digits = consume_digits([](u8 c){ return c == '0' || c == '1'; }); break;
				case 'o': digits = consume_digits([](u8 c){ return c >= '0' && c <= '7'; }); break;
				case 'x': digits = consume_digits(is_hex_digit); break;
			}
			if(digits == 0){
				return result_err(error_at(start, CompErr_MalformedNumber));
			}
		}
		else {
			consume_digits(is_digit);

			// No trailing dot, `1.` and `1..2` are not floats
			if(peek_char() == '.' && is_digit(peek_char(1))){
				current += 1;
				consume_digits(is_digit);
				type = Tk_Float;
			}

			if(consume_if([](u8 c){ return c == 'e' || c == 'E'; })){
				consume_if([](u8 c){ return c == '+' || c == '-'; });
				if(consume_digits(is_digit) == 0){
					return result_err(error_at(start, CompErr_MalformedNumber));
				}
				type = Tk_Float;
			}
		}

		// Catches things like `0b102` or `12abc`
		if(is_ident_char(peek_char())){
			return result_err(error_at(current, CompErr_MalformedNumber, peek_char()));
		}

		return result_ok(make_token(start, type));
	}

	// Validate an escape sequence starting at the backslash
	Result<void, Syntax_Error> lex_escape(){
		usize start = current;
		current += 1;

		u8 c = peek_char();
		if(consume_if([](u8 c){ return c == 't' || c == 'n' || c == 'r' || c == '"' || c == '\'' || c == '\\'; })){
			return result_ok();
		}

		if(!consume_char('u') || !consume_char('{')){
			return result_err(error_at(start, CompErr_InvalidEscape, c));
		}

		usize digits_start = current;
		usize digits = consume_while(is_hex_digit);
		if(digits == 0 || digits > 6 || !consume_char('}')){
			return result_err(error_at(start, CompErr_InvalidCodepoint));
		}

		rune r = 0;
		for(usize i = digits_start; i < digits_start + digits; i += 1){
			r = r * 16 + hex_value(source[i]);
		}
		if(r > 0x10ffff || (r >= 0xd800 && r <= 0xdfff)){
			return result_err(error_at(start, CompErr_InvalidCodepoint, r));
		}

		return result_ok();
	}

	Result<Token, Syntax_Error> lex_string(){
		usize start = current;
		current += 1;

		for(;;){
			u8 c = peek_char();
			if(current >= source.len() || c == '\n' || c == '\r'){
				return result_err(error_at(start, CompErr_UnterminatedString));
			}
			if(c == '\t'){
				return result_err(error_at(current, CompErr_UnexpectedChar, c));
			}

			if(c == '"'){
				current += 1;
				break;
			}
			else if(c == '\\'){
				err_return(lex_escape());
			}
			else {
				err_return(consume_rune());
			}
		}

		return result_ok(make_token(start, Tk_String));
	}

	Result<Token, Syntax_Error> lex_char(){
		usize start = current;
		current += 1;

		u8 c = peek_char();
		if(current >= source.len() || c == '\n' || c == '\r'){
			return result_err(error_at(start, CompErr_UnterminatedChar));
		}
		if(c == '\t'){
			return result_err(error_at(current, CompErr_UnexpectedChar, c));
		}
		if(c == '\''){
			return result_err(error_at(start, CompErr_EmptyChar));
		}

		if(c == '\\'){
			err_return(lex_escape());
		}
		else {
			err_return(consume_rune());
		}

		if(!consume_char('\'')){
			return result_err(error_at(start, CompErr_UnterminatedChar));
		}

		return result_ok(make_token(start, Tk_Char));
	}
};
