#include "testing.hpp"
#include "../base.hpp"
#include "../lexer.hpp"

namespace test {

static Lexer lexer_make(String source){
	return Lexer{ .filepath = "<test>", .source = source, .current = 0 };
}

// `source` produces exactly the `expected` token types followed by EOF
template<usize N>
static bool lexes_to(String source, Token_Type const (&expected)[N]){
	Lexer lexer = lexer_make(source);
	for(usize i = 0; i < N; i += 1){
		auto res = lexer.next();
		if(!res.has_value() || res.unwrap().type != expected[i]){ return false; }
	}
	auto res = lexer.next();
	return res.has_value() && res.unwrap().type == Tk_EndOfFile;
}

// `source` produces no tokens besides EOF
static bool lexes_nothing(String source){
	Lexer lexer = lexer_make(source);
	auto res = lexer.next();
	return res.has_value() && res.unwrap().type == Tk_EndOfFile;
}

// All of `source` is a single token of `type`
static bool lexes_one(String source, Token_Type type){
	Lexer lexer = lexer_make(source);
	auto res = lexer.next();
	if(!res.has_value()){ return false; }
	Token token = res.unwrap();
	return token.type == type && token.start == 0 && token.length == source.len()
		&& lexer.next().unwrap_or({}).type == Tk_EndOfFile;
}

// Lexing `source` eventually fails with `type` at `offset`
static bool fails_with(String source, Compiler_Error_Type type, usize offset){
	Lexer lexer = lexer_make(source);
	for(;;){
		auto res = lexer.next();
		if(!res.has_value()){
			Syntax_Error err = res.unwrap_err();
			return err.type == type && err.offset == offset;
		}
		if(res.unwrap().type == Tk_EndOfFile){ return false; }
	}
}

bool lexer_tests(){
	bool ok = true;

	ok &= test("lexer: empty and whitespace", [](Test& t){
		check(lexes_nothing(""));
		check(lexes_nothing(" \t\r\n  "));

		Lexer lexer = lexer_make("   ");
		Token eof = lexer.next().unwrap();
		check(eof.type == Tk_EndOfFile && eof.start == 3 && eof.length == 0);
		check(lexer.next().unwrap().type == Tk_EndOfFile);
	});

	ok &= test("lexer: identifiers and keywords", [](Test& t){
		check(lexes_one("x", Tk_Identifier));
		check(lexes_one("_", Tk_Identifier));
		check(lexes_one("_foo_Bar9", Tk_Identifier));
		check(lexes_to("var proc if and or not", {Tk_Var, Tk_Proc, Tk_If, Tk_And, Tk_Or, Tk_Not}));

		// Keyword prefixes and suffixes are still identifiers
		check(lexes_to("iff vars _if and_ notx", {Tk_Identifier, Tk_Identifier, Tk_Identifier, Tk_Identifier, Tk_Identifier}));
	});

	ok &= test("lexer: operators", [](Test& t){
		check(lexes_to("( ) [ ] { } . , ; : ? ^", {
			Tk_ParenOpen, Tk_ParenClose, Tk_SquareOpen, Tk_SquareClose, Tk_CurlyOpen, Tk_CurlyClose,
			Tk_Dot, Tk_Comma, Tk_Semicolon, Tk_Colon, Tk_Question, Tk_Caret,
		}));
		check(lexes_to("+ - * / % & | ~ << >>", {
			Tk_Plus, Tk_Minus, Tk_Star, Tk_Slash, Tk_Mod, Tk_BitAnd, Tk_BitOr, Tk_Tilde, Tk_ShiftLeft, Tk_ShiftRight,
		}));
		check(lexes_to("+= -= *= /= %= %%= &= |= ~= <<= >>=", {
			Tk_PlusAssign, Tk_MinusAssign, Tk_StarAssign, Tk_SlashAssign, Tk_ModAssign, Tk_ModFloorAssign,
			Tk_BitAndAssign, Tk_BitOrAssign, Tk_TildeAssign, Tk_ShiftLeftAssign, Tk_ShiftRightAssign,
		}));
		check(lexes_to("= == != < > <= >= ->", {
			Tk_Assign, Tk_Equal, Tk_NotEqual, Tk_Less, Tk_Greater, Tk_LessEqual, Tk_GreaterEqual, Tk_Arrow,
		}));
	});

	ok &= test("lexer: maximal munch", [](Test& t){
		check(lexes_to("a<<=b", {Tk_Identifier, Tk_ShiftLeftAssign, Tk_Identifier}));
		check(lexes_to("<<<", {Tk_ShiftLeft, Tk_Less}));
		check(lexes_to("===", {Tk_Equal, Tk_Assign}));
		check(lexes_to("->>", {Tk_Arrow, Tk_Greater}));
		check(lexes_to("%%%", {Tk_ModFloor, Tk_Mod}));
		check(lexes_to("x-1", {Tk_Identifier, Tk_Minus, Tk_Integer}));
		check(lexes_to("a.b", {Tk_Identifier, Tk_Dot, Tk_Identifier}));
	});

	ok &= test("lexer: integers", [](Test& t){
		check(lexes_one("0", Tk_Integer));
		check(lexes_one("1234567890", Tk_Integer));
		check(lexes_one("1_000_000", Tk_Integer));
		check(lexes_one("0b1010_1010", Tk_Integer));
		check(lexes_one("0o777", Tk_Integer));
		check(lexes_one("0xdead_BEEF", Tk_Integer));
		check(lexes_one("0x_ff", Tk_Integer));

		// Based integers do not have fractions
		check(lexes_to("0x1.5", {Tk_Integer, Tk_Dot, Tk_Integer}));
	});

	ok &= test("lexer: floats", [](Test& t){
		check(lexes_one("1.5", Tk_Float));
		check(lexes_one("0.0", Tk_Float));
		check(lexes_one("1_0.2_5", Tk_Float));
		check(lexes_one("1e10", Tk_Float));
		check(lexes_one("1E10", Tk_Float));
		check(lexes_one("2.5e-3", Tk_Float));
		check(lexes_one("2.5E+3", Tk_Float));

		// No leading or trailing dot
		check(lexes_to("1.", {Tk_Integer, Tk_Dot}));
		check(lexes_to(".5", {Tk_Dot, Tk_Integer}));
		check(lexes_to("1..5", {Tk_Integer, Tk_Dot, Tk_Dot, Tk_Integer}));
		check(lexes_to("x.5", {Tk_Identifier, Tk_Dot, Tk_Integer}));
		check(lexes_to("1.x", {Tk_Integer, Tk_Dot, Tk_Identifier}));
	});

	ok &= test("lexer: malformed numbers", [](Test& t){
		check(fails_with("0x", CompErr_MalformedNumber, 0));
		check(fails_with("0b_", CompErr_MalformedNumber, 0));
		check(fails_with("0b12", CompErr_MalformedNumber, 3));
		check(fails_with("0o8", CompErr_MalformedNumber, 0));
		check(fails_with("0xfg", CompErr_MalformedNumber, 3));
		check(fails_with("12abc", CompErr_MalformedNumber, 2));
		check(fails_with("1e", CompErr_MalformedNumber, 0));
		check(fails_with("1e+", CompErr_MalformedNumber, 0));
		check(fails_with("1.5x", CompErr_MalformedNumber, 3));
	});

	ok &= test("lexer: strings", [](Test& t){
		check(lexes_one("\"\"", Tk_String));
		check(lexes_one("\"hello world\"", Tk_String));
		check(lexes_one("\"\\t\\n\\r\\\"\\'\\\\\"", Tk_String));
		check(lexes_one("\"\\u{fffd}\"", Tk_String));
		check(lexes_one("\"\\u{0}\\u{10ffff}\\u{1F600}\"", Tk_String));
		check(lexes_one("\"caf\xc3\xa9 \xf0\x9f\x98\x80\"", Tk_String));
		check(lexes_to("\"a\" \"b\"", {Tk_String, Tk_String}));
	});

	ok &= test("lexer: string errors", [](Test& t){
		check(fails_with("\"abc", CompErr_UnterminatedString, 0));
		check(fails_with("x \"ab\ncd\"", CompErr_UnterminatedString, 2));
		check(fails_with("\"ab\rcd\"", CompErr_UnterminatedString, 0));
		check(fails_with("\"a\tb\"", CompErr_UnexpectedChar, 2));
		check(fails_with("\"\\q\"", CompErr_InvalidEscape, 1));
		check(fails_with("\"\\u0041\"", CompErr_InvalidEscape, 1));
		check(fails_with("\"\\u{}\"", CompErr_InvalidCodepoint, 1));
		check(fails_with("\"\\u{1234567}\"", CompErr_InvalidCodepoint, 1));
		check(fails_with("\"\\u{ff\"", CompErr_InvalidCodepoint, 1));
		check(fails_with("\"\\u{110000}\"", CompErr_InvalidCodepoint, 1));
		check(fails_with("\"\\u{d800}\"", CompErr_InvalidCodepoint, 1));
		check(fails_with("\"\\u{dfff}\"", CompErr_InvalidCodepoint, 1));
		check(fails_with("\"a\xff\"", CompErr_InvalidUtf8, 2));
		check(fails_with("\"\xc3\"", CompErr_InvalidUtf8, 1));
	});

	ok &= test("lexer: chars", [](Test& t){
		check(lexes_one("'a'", Tk_Char));
		check(lexes_one("'\"'", Tk_Char));
		check(lexes_one("'\\''", Tk_Char));
		check(lexes_one("'\\n'", Tk_Char));
		check(lexes_one("'\\u{1f600}'", Tk_Char));
		check(lexes_one("'\xc3\xa9'", Tk_Char));

		check(fails_with("''", CompErr_EmptyChar, 0));
		check(fails_with("'ab'", CompErr_UnterminatedChar, 0));
		check(fails_with("'a", CompErr_UnterminatedChar, 0));
		check(fails_with("'", CompErr_UnterminatedChar, 0));
		check(fails_with("'\n'", CompErr_UnterminatedChar, 0));
		check(fails_with("'\t'", CompErr_UnexpectedChar, 1));
		check(fails_with("'\\x'", CompErr_InvalidEscape, 1));
	});

	ok &= test("lexer: comments", [](Test& t){
		check(lexes_nothing("// just a comment"));
		check(lexes_to("a // b c\nd", {Tk_Identifier, Tk_Identifier}));
		check(lexes_to("a /* b */ c", {Tk_Identifier, Tk_Identifier}));
		check(lexes_to("a /* /* nested */ still */ c", {Tk_Identifier, Tk_Identifier}));
		check(lexes_to("a/**/b", {Tk_Identifier, Tk_Identifier}));
		check(lexes_to("a / b", {Tk_Identifier, Tk_Slash, Tk_Identifier}));
		check(lexes_to("\"// not a comment\"", {Tk_String}));

		check(fails_with("a /* b", CompErr_UnterminatedComment, 2));
		check(fails_with("/* /* */", CompErr_UnterminatedComment, 0));
	});

	ok &= test("lexer: unexpected chars", [](Test& t){
		check(fails_with("!", CompErr_UnexpectedChar, 0));
		check(fails_with("a $", CompErr_UnexpectedChar, 2));
		check(fails_with("@", CompErr_UnexpectedChar, 0));
		check(fails_with("#", CompErr_UnexpectedChar, 0));
		check(fails_with("`", CompErr_UnexpectedChar, 0));

		// Non-ASCII reports the decoded codepoint
		Lexer lexer = lexer_make("\xc3\xa9");
		Syntax_Error err = lexer.next().unwrap_err();
		check(err.type == CompErr_UnexpectedChar && err.got.codepoint == 0xe9);
	});

	ok &= test("lexer: token spans", [](Test& t){
		String source = "var  x := 0x1f";
		Lexer lexer = lexer_make(source);
		Token tokens[5] = {};
		for(usize i = 0; i < 5; i += 1){
			tokens[i] = lexer.next().unwrap();
		}

		check(tokens[0].type == Tk_Var && tokens[0].start == 0 && tokens[0].length == 3);
		check(tokens[1].type == Tk_Identifier && tokens[1].start == 5 && tokens[1].length == 1);
		check(tokens[2].type == Tk_Colon && tokens[2].start == 7 && tokens[2].length == 1);
		check(tokens[3].type == Tk_Assign && tokens[3].start == 8 && tokens[3].length == 1);
		check(tokens[4].type == Tk_Integer && tokens[4].start == 10 && tokens[4].length == 4);
		check(source.slice(tokens[4].start, tokens[4].start + tokens[4].length) == "0x1f");
	});

	ok &= test("lexer: peek", [](Test& t){
		Lexer lexer = lexer_make("a + b");
		check(lexer.peek().unwrap().type == Tk_Identifier);
		check(lexer.peek().unwrap().type == Tk_Identifier);
		check(lexer.next().unwrap().type == Tk_Identifier);
		check(lexer.peek().unwrap().type == Tk_Plus);
		check(lexer.next().unwrap().type == Tk_Plus);
		check(lexer.next().unwrap().type == Tk_Identifier);
		check(lexer.peek().unwrap().type == Tk_EndOfFile);
		check(lexer.next().unwrap().type == Tk_EndOfFile);

		// Peeking an error does not advance either
		Lexer bad = lexer_make("!");
		check(!bad.peek().has_value());
		check(bad.current == 0);
	});

	ok &= test("lexer: error location", [](Test& t){
		Lexer lexer = lexer_make("a\nb\n  \"oops");
		Syntax_Error err = {};
		for(;;){
			auto res = lexer.next();
			if(!res.has_value()){
				err = res.unwrap_err();
				break;
			}
		}
		check(err.type == CompErr_UnterminatedString);
		check(err.offset == 6);
		check(err.line == 3);
		check(err.filepath == "<test>");
	});

	return ok;
}

}
