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
	Result<Token, Syntax_Error> next();

	// Get the next token without advancing
	Result<Token, Syntax_Error> peek();

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

	Token make_token(usize start, Token_Type type){
		return Token{start, u32(current - start), type};
	}

	// Consume a single UTF-8 encoded codepoint
	Result<void, Syntax_Error> consume_rune();

	Syntax_Error error_at(usize offset, Compiler_Error_Type type, rune got = 0);

	Result<void, Syntax_Error> skip_whitespace_and_comments();

	// Longest operator matching the current position
	Result<Token, Syntax_Error> lex_operator();

	Result<Token, Syntax_Error> lex_number();

	// Validate an escape sequence starting at the backslash
	Result<void, Syntax_Error> lex_escape();

	Result<Token, Syntax_Error> lex_string();

	Result<Token, Syntax_Error> lex_char();
};
