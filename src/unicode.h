#ifndef _RJ_STRINGS
#define _RJ_STRINGS

#include <stdint.h>
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;
typedef u32 glyph;

typedef struct {
	u32* chars;
	size_t len;
} utf32;
typedef struct {
	u16* chars;
	size_t len;
} utf16;
typedef struct {
	u8*  chars;
	size_t len;
} utf8;

typedef struct{
	glyph glyph;
	u32 consumed;
} glyph_return;

glyph_return glyph_from_utf8(utf8 text);
glyph_return glyph_from_utf16(utf16 text);
glyph_return glyph_from_utf32(utf32 text);

size_t glyph_to_utf16(glyph g, utf16 text);
size_t utf8_to_utf16(utf8 in, utf16 out);

size_t utf8_glyph_count(utf8 text);
size_t utf16_glyph_count(utf16 text);
size_t utf32_glyph_count(utf32 text);

#ifdef RJ_STRINGS_IMPL

#ifdef RJ_ASSERT
	#define assert RJ_ASSERT
#else
	#include <assert.h>
#endif

#ifndef RJ_TMP_ALLOC
	#ifndef RJ_TMP_ALLOC_SIZE
		#define RJ_TMP_ALLOC_SIZE 1024
	#endif
	u8 _tmp[RJ_TMP_ALLOC_SIZE];
	void* RJ_TMP_ALLOC(size_t len){
		assert(len <= RJ_TMP_ALLOC_SIZE);
		return _tmp;
	}
#endif
#ifndef RJ_TMP_ALLOC_FREE
	void RJ_TMP_ALLOC_FREE(void){}
#endif

#if defined(__GNUC__)
	#define FORCE_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
	#define FORCE_INLINE __forceinline inline
#else
	// hope this works
	#define inline
#endif


FORCE_INLINE glyph_return glyph_from_utf32(utf32 text){return (glyph_return){text.chars[0], 1};}
glyph_return glyph_from_utf16(utf16 text){
	assert(0 && "NOT IMPLIMENTED");
	return (glyph_return){0};
}
glyph_return glyph_from_utf8(utf8 text){
	glyph rv;
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
	return (glyph_return){rv, n};
}

#define range(val, min, max) (((val) >= (min) && (val) <= (max)))

size_t glyph_to_utf16(glyph g, utf16 text){
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
		if (text.chars != NULL){text.chars[n] = 0b110110|((g&0b11111111110000000000)>>10);}
		n++;
		if (text.chars != NULL){text.chars[n] = 0b110111|((g&0b00000000001111111111));}
		n++;
	}else{
		assert(text.chars == NULL || text.len >= 1);
		if (text.chars != NULL){text.chars[n] = 0xFFFD;}
		n++;
	}
	return n;
}

size_t utf8_to_utf16(utf8 in, utf16 out){
	assert(in.chars);
	int glyphs = 0;
	for (size_t len = in.len; len > 0; glyphs++){
		glyph_return g = glyph_from_utf8((utf8){in.chars+in.len-len, len});
		len -= g.consumed;
	}
	u32* t = RJ_TMP_ALLOC(glyphs*sizeof(glyph));
	for (int i = 0; in.len > 0; i++){
		glyph_return g = glyph_from_utf8(in);
		t[i] = g.glyph;
		in.len -= g.consumed;
		in.chars += g.consumed;
	}
	size_t n = 0;
	utf16 text = {0};
	for(int i = 0; i < glyphs; i++){
		if (out.chars!=NULL){
			text = (utf16){out.chars+n, out.len-n};
		}
		n += glyph_to_utf16(t[i], text);
	}
	RJ_TMP_ALLOC_FREE();
	return n;
}

size_t utf8_glyph_count(utf8 text){
	assert(text.chars);
	size_t glyphs = 0;
	for (size_t len = text.len; len > 0; glyphs++){
		glyph_return g = glyph_from_utf8((utf8){text.chars+text.len-len, len});
		len -= g.consumed;
	}
	return glyphs;
}
size_t utf16_glyph_count(utf16 text){
	assert(0 && "NOT IMPLIMENTED");
	return 0;
}
FORCE_INLINE size_t utf32_glyph_count(utf32 text){return text.len;}


#endif // RJ_STRINGS_IMPL
#endif // _RJ_STRINGS