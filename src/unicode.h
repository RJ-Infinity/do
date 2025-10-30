#ifndef _RJ_STRINGS
#define _RJ_STRINGS

#include <stdint.h>
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;
typedef u32 codepoint;

typedef struct {
	u32* chars;
	size_t len;
} utf32;
typedef struct {
	u16* chars;
	size_t len;
} utf16;
typedef struct {
	u8* chars;
	size_t len;
} utf8;

typedef struct{
	codepoint codepoint;
	u32 consumed;
} codepoint_return;

codepoint_return codepoint_from_utf8(utf8 text);
codepoint_return codepoint_from_utf16(utf16 text);
codepoint_return codepoint_from_utf32(utf32 text);


size_t codepoint_to_utf8(codepoint g, utf8 text);
size_t codepoint_to_utf16(codepoint g, utf16 text);
size_t codepoint_to_utf32(codepoint g, utf32 text);


size_t utf8_to_utf16(utf8 in, utf16 out);
size_t utf8_to_utf32(utf8 in, utf32 out);

size_t utf16_to_utf8(utf16 in, utf8 out);
size_t utf16_to_utf32(utf16 in, utf32 out);

size_t utf32_to_utf8(utf32 in, utf8 out);
size_t utf32_to_utf16(utf32 in, utf16 out);


size_t utf8_codepoint_count(utf8 text);
size_t utf16_codepoint_count(utf16 text);
size_t utf32_codepoint_count(utf32 text);
#endif // _RJ_STRINGS
#ifdef RJ_STRINGS_IMPL
#undef RJ_STRINGS_IMPL

#include <stddef.h>

#ifdef RJ_ASSERT
	#define assert(cond) RJ_ASSERT(cond)
#else
	#include <assert.h>
#endif
#define range(val, min, max) (((val) >= (min) && (val) <= (max)))


codepoint_return codepoint_from_utf8(utf8 text){
	codepoint rv;
	int n = 0;
	assert(text.chars);
	assert(text.len >= 1);
	if (text.chars[n] >> 3 == 0b11110){
		assert(text.len >= 4);
		u32 b1 = text.chars[n];
		n++;
		u32 b2 = text.chars[n];
		n++;
		u32 b3 = text.chars[n];
		n++;
		u32 b4 = text.chars[n];
		n++;
		rv = (b1&0b111)<<18 | (b2&0b111111)<<12 | (b3&0b111111)<<6 | (b4&0b111111);
	}else if (text.chars[n] >> 4 == 0b1110){
		assert(text.len >= 3);
		u32 b1 = text.chars[n];
		n++;
		u32 b2 = text.chars[n];
		n++;
		u32 b3 = text.chars[n];
		n++;
		rv = (b1&0b1111)<<12 | (b2&0b111111)<<6 | (b3&0b111111);
	}else if (text.chars[n] >> 5 == 0b110){
		assert(text.len >= 2);
		u32 b1 = text.chars[n];
		n++;
		u32 b2 = text.chars[n];
		n++;
		rv = (b1&0b11111)<<6 | (b2&0b111111);
	}else{
		rv = text.chars[n];
		n++;
	}
	return (codepoint_return){rv, n};
}
codepoint_return codepoint_from_utf16(utf16 text){
	codepoint rv;
	int n = 0;
	assert(text.chars);
	assert(text.len >= 1);
	if (
		range(text.chars[n], 0x0000, 0xD7FF) ||
		range(text.chars[n], 0xE000, 0xFFFF)
	){
		rv = text.chars[n];
		n++;
	}else if(
		(text.chars[n] >> 10) == 0b110110 &&
		(assert(text.len >= 2), true) &&
		(text.chars[n+1] >> 10) == 0b110111
	){
		rv = 0x10000 + (
			((codepoint)((text.chars[n+0] & 0b1111111111) << 10)) |
			((codepoint)((text.chars[n+1] & 0b1111111111) << 00))
		);
		n += 2;
	}else{ // replacement character
		n++;
		rv = 0xFFFD;
	}
	return (codepoint_return){rv, n};
}
codepoint_return codepoint_from_utf32(utf32 text){return (codepoint_return){text.chars[0], 1};}


size_t codepoint_to_utf8(codepoint g, utf8 text){
	int n = 0;
	if (range(g, 0x000000, 0x00007F)){
		if (text.chars != NULL){
			assert(text.len >= 1);
			text.chars[n] = (u8)g;
		}
		n+=1;
	}else if (range(g, 0x000080, 0x0007FF)){
		if (text.chars != NULL){
			assert(text.len >= 2);
			text.chars[n+0] = (u8)(((g>> 6) & 0b11111)|(0b110<<5));
			text.chars[n+1] = (u8)(((g>> 0) & 0b111111)|(0b10<<6));
		}
		n+=2;
	}else if (range(g, 0x000800, 0x00FFFF)){
		if (text.chars != NULL){
			assert(text.len >= 3);
			text.chars[n+0] = (u8)(((g>>12) & 0b1111)|(0b1110<<4));
			text.chars[n+1] = (u8)(((g>> 6) & 0b111111)|(0b10<<6));
			text.chars[n+2] = (u8)(((g>> 0) & 0b111111)|(0b10<<6));
		}
		n+=3;
	}else if (range(g, 0x010000, 0x10FFFF)){
		if (text.chars != NULL){
			assert(text.len >= 3);
			text.chars[n+0] = (u8)(((g>>18) & 0b111)|(0b11110<<3));
			text.chars[n+1] = (u8)(((g>>12) & 0b111111)|(0b10<<6));
			text.chars[n+2] = (u8)(((g>> 6) & 0b111111)|(0b10<<6));
			text.chars[n+3] = (u8)(((g>> 0) & 0b111111)|(0b10<<6));
		}
		n+=4;
	}else{ // replacement character
		if (text.chars != NULL){
			assert(text.len >= 3);
			text.chars[n+0] = 0xEF;
			text.chars[n+1] = 0xBF;
			text.chars[n+2] = 0xBD;
		}
		n+=3;
	}
	return n;
}
size_t codepoint_to_utf16(codepoint g, utf16 text){
	int n = 0;
	if (
		range(g, 0x0000, 0xD7FF) ||
		range(g, 0xE000, 0xFFFF)
	){
		assert(text.chars == NULL || text.len >= 1);
		if (text.chars != NULL){text.chars[n] = g;}
		n++;
	}else if (range(g, 0x010000, 0x10FFFF)){
		assert(text.chars == NULL || text.len >= 2);
		g -= 0x10000;
		if (text.chars != NULL){text.chars[n] = (0b110110<<10)|((g&0b11111111110000000000)>>10);}
		n++;
		if (text.chars != NULL){text.chars[n] = (0b110111<<10)|((g&0b00000000001111111111)>>00);}
		n++;
	}else{ // replacement character
		assert(text.chars == NULL || text.len >= 1);
		if (text.chars != NULL){text.chars[n] = 0xFFFD;}
		n++;
	}
	return n;
}
size_t codepoint_to_utf32(codepoint g, utf32 text){
	assert(text.chars == NULL || text.len >= 1);
	if (text.chars != NULL){text.chars[0] = g;}
	return 1;
}

#define _utf_convert(from, to) \
size_t from##_to_##to(from in, to out){\
	assert(in.chars);\
	size_t n = 0;\
	to text = {0};\
	for(size_t len = in.len; len > 0;){\
		codepoint_return g = codepoint_from_##from((from){in.chars+in.len-len, len});\
		len -= g.consumed;\
		if (out.chars != NULL){\
			text = (to){out.chars+n, out.len-n};\
		}\
		n += codepoint_to_##to(g.codepoint, text);\
	}\
	return n;\
}\

_utf_convert(utf8, utf16)
_utf_convert(utf8, utf32)

_utf_convert(utf16, utf8)
_utf_convert(utf16, utf32)

_utf_convert(utf32, utf8)
_utf_convert(utf32, utf16)


size_t utf8_codepoint_count(utf8 text){
	assert(text.chars);
	size_t codepoints = 0;
	for (size_t len = text.len; len > 0; codepoints++){
		codepoint_return g = codepoint_from_utf8((utf8){text.chars+text.len-len, len});
		len -= g.consumed;
	}
	return codepoints;
}
size_t utf16_codepoint_count(utf16 text){
	assert(text.chars);
	size_t codepoints = 0;
	for (size_t len = text.len; len > 0; codepoints++){
		codepoint_return g = codepoint_from_utf16((utf16){text.chars+text.len-len, len});
		len -= g.consumed;
	}
	return codepoints;
}
size_t utf32_codepoint_count(utf32 text){return text.len;}


#endif // RJ_STRINGS_IMPL