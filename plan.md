# Lexer plan

> **Process:** Stop after every major step for review. Commit after every minor step.
> Commit messages are short and follow the repo convention: a brief capitalized phrase with
> no prefix or trailing period (e.g. `Token types`, `Result type`, `Simplify result signatures`),
> followed by the Co-Authored-By trailer.

## Context
`main.cpp` has the token X-macro tables, `Token`, `Syntax_Error` and an empty `Lexer` struct. We need a pull-based lexer that the parser will drive via `next()` / `peek()`, each returning `Result<Token, Syntax_Error>`. Decisions from Q&A:
- Add `=` (assign). Logical ops are **keywords only**: `and`, `or`, `not`. `~` stays as the one token for both XOR and bitwise NOT.
- Comments: `//` to end of line only.
- Errors: stop at first error (returned from `next()`); no accumulation in the lexer itself.
- Numbers: decimal, `0x`/`0b`/`0o`, `_` separators; reals are decimal with `.` and/or `e`/`E[+-]`; **no leading or trailing `.`** (`.5` → `Dot Integer`; `1.` / `1.x` → error).
- Strings: `"..."`, single-line, escapes `\n \r \t \u{...}` (plus `\\` and `\"`, which are needed to write those characters at all). The lexer only validates; the token is the source span and decoding happens later.

## Steps

**Major step 1: Token table and errors**
1. Move token/error/lexer declarations from `main.cpp` into `lexer.hpp`, add `#include "lexer.hpp"` / `#include "lexer.cpp"` (empty stub) to `main.cpp`. Commit `Move tokens to lexer header`.
2. Swap the Paren/Curly names. Commit `Fix paren and curly token names`.
3. Add `Assign`, `LogicAnd/Or/Not` keywords, `EndOfFile`. Commit `Add assign, logic keywords and EOF tokens`.
4. Add the `Compiler_Error_Type` entries. Commit `Lexer error types`.
5. Fix `STRF`. Commit `Fix STRF macro`.
→ stop

**Major step 2: Lexer core**
1. `Lexer` struct, `lexer_make`, byte helpers, `token_text`. Commit `Lexer scaffolding`.
2. Trivia skipping (whitespace, line counting, `//`). Commit `Skip whitespace and comments`.
3. Separators with maximal munch. Commit `Lex separators`.
4. Identifiers and keywords (with UTF-8). Commit `Lex identifiers and keywords`.
5. Numbers. Commit `Lex numbers`.
6. Strings and escapes. Commit `Lex strings`.
7. `peek()`. Commit `Lexer peek`.
→ stop

**Major step 3: Tests and demo**
1. `tests/lexer.cpp` with `lex_all` and wiring into `tests/main.cpp`; one commit per test group (separators, identifiers/keywords, numbers, strings, comments/lines/peek/errors), e.g. `Lexer separator tests`.
2. `entrypoint()` demo that prints tokens. Commit `Print tokens in entrypoint`.
→ stop

The sections below are the design details for these steps.

## File layout
Move the lexer out of `main.cpp` so tests can include it (tests/main.cpp has its own `main`):
- **New `lexer.hpp`**: the token X-macros, `Token_Type`, `token_type_name`, `Token`, `Compiler_Error_Type`, `Syntax_Error`, `Lexer` declaration.
- **New `lexer.cpp`**: the implementation.
- **`main.cpp`**: `#include "lexer.hpp"` at top, `#include "lexer.cpp"` next to `base.cpp` at the bottom (same unity-build style). `entrypoint()` lexes a sample string and prints the tokens.
- **`Makefile`**: `lexer.*` is already picked up by `$(wildcard *.cpp *.hpp)`; no change needed.

## Token table changes (lexer.hpp)
- Fix the swapped names: `CurlyOpen/Close` currently map to `(`/`)` and `ParenOpen/Close` to `{`/`}`. Swap so `Paren` = `()`, `Curly` = `{}`.
- Add `X(Assign, "=")` to `TOKENS_SEP`.
- Add `X(LogicAnd, "and") X(LogicOr, "or") X(LogicNot, "not")` to `TOKENS_KEYWORDS` (prefixed to avoid clashing with existing bitwise `Tk_And`/`Tk_Or`).
- Add `X(EndOfFile, "<EOF>")` to `TOKENS`. `next()` at end of input returns it every time it's called.

## Error types
```cpp
enum Compiler_Error_Type : u16 {
	CompErr_None = 0,
	CompErr_Unexpected_Char,     // got.codepoint = offending rune (e.g. lone `!`, `@`)
	CompErr_Invalid_UTF8,
	CompErr_Unterminated_String, // newline or EOF before closing quote
	CompErr_Invalid_Escape,      // got.codepoint = char after backslash
	CompErr_Invalid_Codepoint,   // \u{} empty, >6 hex digits, >0x10FFFF, or surrogate
	CompErr_Malformed_Number,    // trailing `.`, no digits after 0x/exponent, bad digit for base, trailing `_`, letter glued to number
};
```
`Syntax_Error` gets `filepath`, `offset` (byte offset of the error), `line`, `type`, and `expected`/`got` where meaningful.

## Lexer
```cpp
struct Lexer {
	String filepath;
	String source;
	usize current;
	i32 line;          // 1-based

	Result<Token, Syntax_Error> next();
	Result<Token, Syntax_Error> peek(); // copies *this, calls next() on the copy
	String token_text(Token t);         // source.slice(t.start, t.start + t.length)
};
Lexer lexer_make(String filepath, String source);
```
`peek()` is just state save/restore — `Lexer` is a small POD, so `Lexer tmp = *this; return tmp.next();`.

### `next()` algorithm
1. **Skip trivia**: loop over ` `, `\t`, `\r`, `\n` (increment `line`), and `//` up to (not consuming past) `\n`.
2. At end → `Tk_EndOfFile` with `start = source.len()`, `length = 0`.
3. Dispatch on first byte:
   - `[A-Za-z_]` or byte ≥ 0x80 → **identifier**: decode with `rune_decode` (base.hpp:683); `RUNE_ERROR` with size 1 → `CompErr_Invalid_UTF8`. Continue while `[A-Za-z0-9_]` or non-ASCII rune. Then check keywords by expanding `TOKENS_KEYWORDS` into a chain of `if(text == String(str)) type = Tk_##sym;` (String::operator== exists, base.hpp:648).
   - `[0-9]` → **number** (below).
   - `"` → **string** (below).
   - Otherwise → **separator** via a `switch` with maximal munch:
     `( ) [ ] { } : ; , . + * / & | ~` single; `-` / `->`; `<` / `<<` / `<=`; `>` / `>>` / `>=`; `=` / `==`; `!=` (lone `!` → `CompErr_Unexpected_Char`). `/` is only reached when not followed by `/` (comments were skipped in step 1).
   - Anything else → `CompErr_Unexpected_Char` with the decoded rune in `got.codepoint`.

### Numbers
- If `0` followed by `x`/`b`/`o`: consume prefix, then digits of that base with `_` allowed. Require ≥1 digit; `_` may not be last. Result `Tk_Integer`.
- Else decimal digits (+`_`). Then:
  - `.` followed by a digit → fractional digits, mark real.
  - `.` followed by anything else → `CompErr_Malformed_Number` (no trailing dot).
  - `e`/`E`, optional `+`/`-`, ≥1 digit → mark real.
- After any number, if the next char is an identifier char (letter, `_`, digit of wrong base, ≥0x80) → `CompErr_Malformed_Number` (catches `123abc`, `0b102`, `0xZZ`).
- Leading `.` needs no special handling: `.` dispatches to `Tk_Dot` and the digits become an Integer.

### Strings
Consume `"`, then loop: `"` ends; `\n` or EOF → `CompErr_Unterminated_String` (offset = opening quote); `\` → one of `n r t \ "`, or `u{` 1–6 hex digits `}` validated ≤ 0x10FFFF and not in 0xD800–0xDFFF; else `CompErr_Invalid_Escape`. Non-ASCII bytes are validated with `rune_decode`. Token span includes the quotes.

### Helpers
Small private helpers in lexer.cpp: `peek_byte(offset)` returning 0 past end, `is_alpha`, `is_digit`, `is_hex_digit`, `digit_in_base`, `make_token(start, type)`, `make_error(type, offset)`. Use `String::operator[]` / `raw_data()` + `len()` from base.hpp.

Note: `STRF` in base.hpp:662 references `.len`/`.v` and won't compile with current `String`; fix it to `(int)(S).len(), (S).raw_data()` (major step 1.5).

## Tests — `tests/lexer.cpp`
Follow existing pattern (`test::test(name, lambda)`, `check(...)`, see tests/string.cpp). Add a test-local `lex_all(String src, Allocator a) -> Result<Dyn_Array<Token>, Syntax_Error>` that drives `next()` until EOF. Wire into `tests/main.cpp` (`#include "../lexer.cpp"` before `base.cpp`, add `ok &= test::lexer_tests();`). Use a stack `Arena` buffer for the allocator like other tests. Cases:
- Every separator, including maximal munch (`<<=` → `ShLeft Assign`, `->`, `!=`, `==` vs `=`).
- Keywords vs identifiers (`if`, `iff`, `and`, `android`, `_x1`, unicode identifier `ñandú`).
- Numbers: `0`, `1_000`, `0xFF_FF`, `0b1010`, `0o17`, `1.5`, `1e10`, `2.5e-3`; errors `1.`, `1.x`, `0x`, `0b2`, `1_`, `123abc`, `1e`.
- `.5` → `Dot Integer`.
- Strings: `""`, `"a\nb"`, `"\u{1F600}"`, `"\""`; errors: unterminated, newline inside, `\q`, `\u{}`, `\u{110000}`, `\u{D800}`.
- Comments and line tracking: error on line 3 reports `line == 3`.
- `peek()` doesn't advance; repeated `next()` at end yields EOF.
- Lone `!`, `@` → `CompErr_Unexpected_Char`.

## Verification
- `make test` — all suites including new lexer tests pass.
- `make run` — `entrypoint()` lexes a small sample program and prints `type_name 'text'` per token; eyeball output.
