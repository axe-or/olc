#pragma once
#include "base.hpp"

#define TOKENS_SEP \
	X(ParenOpen, "(") \
	X(ParenClose, ")") \
	X(SquareOpen, "[") \
	X(SquareClose, "]") \
	X(CurlyOpen, "{") \
	X(CurlyClose, "}") \
	X(Colon, ":") \
	X(Semicolon, ";") \
	X(Comma, ",") \
	X(Dot, ".") \
	X(Arrow, "->") \
	X(Plus, "+") \
	X(Minus, "-") \
	X(Star, "*") \
	X(Slash, "/") \
	X(And, "&") \
	X(Or, "|") \
	X(Tilde, "~") \
	X(ShLeft, "<<") \
	X(ShRight, ">>") \
	X(Gt, ">") \
	X(GtEq, ">=") \
	X(Lt, "<") \
	X(LtEq, "<=") \
	X(Eq, "==") \
	X(Neq, "!=") \
	X(Assign, "=")

#define TOKENS_KEYWORDS \
	X(If, "if") \
	X(Else, "else") \
	X(For, "for") \
	X(Proc, "proc") \
	X(LogicAnd, "and") \
	X(LogicOr, "or") \
	X(LogicNot, "not") \

#define TOKENS \
	TOKENS_SEP \
	TOKENS_KEYWORDS \
	X(Identifier, "Identifier") \
	X(Integer, "Integer") \
	X(Real, "Real") \
	X(String, "String") \
	X(EndOfFile, "<EOF>") \

enum Token_Type : u8 {
	Tk_Unknown = 0,

	#define X(sym, str) Tk_##sym,
	TOKENS
	#undef X
};

constexpr
String token_type_name(Token_Type t){
	switch(t){
		case Tk_Unknown: return "<Unknown>";
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

	CompErr_Unexpected_Char,     // Character that cannot start a token
	CompErr_Invalid_UTF8,        // Malformed UTF-8 sequence
	CompErr_Unterminated_String, // Newline or end of file before closing quote
	CompErr_Invalid_Escape,      // Unknown escape sequence in string
	CompErr_Invalid_Codepoint,   // Bad \u{...} escape: empty, too long, out of range or a surrogate
	CompErr_Malformed_Number,    // Number literal that does not follow the grammar
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
	String filepath;
	String source;
	usize current;
	i32 line; // 1-based line of `current`

	// Produce the next token and advance past it, returns Tk_EndOfFile once the source is exhausted
	Result<Token, Syntax_Error> next();

	// Produce the next token without advancing
	Result<Token, Syntax_Error> peek();

	// Source text of a token
	String token_text(Token t);
};

Lexer lexer_make(String filepath, String source);
