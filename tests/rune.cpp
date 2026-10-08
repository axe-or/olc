#include "testing.hpp"
#include "../base.hpp"

namespace test {

static bool encodes_to(rune r, u8 const* bytes, i32 size){
	Rune_Encoded e = rune_encode(r);
	if(e.size != size){ return false; }
	for(i32 i = 0; i < size; i++){
		if(e.bytes[i] != bytes[i]){ return false; }
	}
	return true;
}

static bool decodes_to(u8 const* bytes, u32 len, rune r, i32 size){
	Rune_Decoded d = rune_decode(bytes, len);
	return d.codepoint == r && d.size == size;
}

bool rune_tests(){
	bool ok = true;
	ok &= test("rune: encode", [](Test& t){
		u8 const ascii[] = {0x41};
		u8 const two[]   = {0xc3, 0xa9};
		u8 const three[] = {0xe2, 0x82, 0xac};
		u8 const four[]  = {0xf0, 0x9f, 0x98, 0x80};
		u8 const error[] = {0xef, 0xbf, 0xbd};

		check(encodes_to('A', ascii, 1));
		check(encodes_to(0xe9, two, 2));
		check(encodes_to(0x20ac, three, 3));
		check(encodes_to(0x1f600, four, 4));

		// Width boundaries
		check(rune_encode(0x7f).size == 1);
		check(rune_encode(0x80).size == 2);
		check(rune_encode(0x7ff).size == 2);
		check(rune_encode(0x800).size == 3);
		check(rune_encode(0xffff).size == 3);
		check(rune_encode(0x10000).size == 4);
		check(rune_encode(0x10ffff).size == 4);

		// Surrogates and out of range values become RUNE_ERROR
		check(encodes_to(0xd800, error, 3));
		check(encodes_to(0xdfff, error, 3));
		check(encodes_to(0x110000, error, 3));
		check(encodes_to(RUNE_ERROR, error, 3));
	});

	ok &= test("rune: decode", [](Test& t){
		u8 const ascii[] = {'z', 'q'};
		u8 const two[]   = {0xc3, 0xa9};
		u8 const three[] = {0xe2, 0x82, 0xac};
		u8 const four[]  = {0xf0, 0x9f, 0x98, 0x80};

		check(decodes_to(ascii, 2, 'z', 1));
		check(decodes_to(two, 2, 0xe9, 2));
		check(decodes_to(three, 3, 0x20ac, 3));
		check(decodes_to(four, 4, 0x1f600, 4));

		Rune_Decoded nothing = rune_decode(ascii, 0);
		check(nothing.codepoint == 0 && nothing.size == 0);
	});

	ok &= test("rune: decode invalid", [](Test& t){
		u8 const continuation[] = {0x80};
		u8 const overlong2[]    = {0xc0, 0x80};
		u8 const overlong3[]    = {0xe0, 0x80, 0x80};
		u8 const surrogate[]    = {0xed, 0xa0, 0x80};
		u8 const too_big[]      = {0xf4, 0x90, 0x80, 0x80};
		u8 const bad_cont[]     = {0xe2, 0x82, 0x41};
		u8 const invalid[]      = {0xff};

		check(decodes_to(continuation, 1, RUNE_ERROR, 1));
		check(decodes_to(overlong2, 2, RUNE_ERROR, 1));
		check(decodes_to(overlong3, 3, RUNE_ERROR, 1));
		check(decodes_to(surrogate, 3, RUNE_ERROR, 1));
		check(decodes_to(too_big, 4, RUNE_ERROR, 1));
		check(decodes_to(bad_cont, 3, RUNE_ERROR, 1));
		check(decodes_to(invalid, 1, RUNE_ERROR, 1));

		// Truncated sequences
		u8 const four[] = {0xf0, 0x9f, 0x98, 0x80};
		check(decodes_to(four, 3, RUNE_ERROR, 1));
		check(decodes_to(four, 1, RUNE_ERROR, 1));
	});

	ok &= test("rune: round trip every codepoint", [](Test& t){
		usize failures = 0;
		for(rune r = 0; r <= 0x10ffff; r++){
			if(r >= 0xd800 && r <= 0xdfff){ continue; }
			Rune_Encoded e = rune_encode(r);
			Rune_Decoded d = rune_decode(e.bytes, u32(e.size));
			if(d.codepoint != r || d.size != e.size){ failures += 1; }
		}
		check(failures == 0);
	});
	return ok;
}

}
