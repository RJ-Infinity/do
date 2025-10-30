#ifndef _RJ_FONT_RENDERER
#define _RJ_FONT_RENDERER
#include <freetype/freetype.h>
#include "unicode.h"

#define max(a, b) (((a) > (b)) ? (a) : (b))

typedef struct TextSize{
	/// all values are in 64th of a pixels (to convert to pixels use `v>>6`)
	FT_Long width;
	FT_Long height;
	// a positive value. can be thought of as the size bellow the baseline
	FT_Long baseline;
} TextSize;
typedef struct TextBitmap{
	uint8_t* buf;
	size_t buf_len;
	TextSize size;
} TextBitmap;

TextBitmap createTextBitmap(TextSize size);
void TextBitmap_destroy(TextBitmap tb);
TextSize render_text(FT_Face face, utf8 text, TextBitmap* image, FT_Pos offset);

#if defined(ANDROID)
#include <android/font_matcher.h>

typedef AFontMatcher* font_matching_data;
typedef struct{
	size_t index;
	const char* file;
	AFont* _font;
} matched_font;
#elif defined(_WIN32)
#include <windows.h>

typedef void* font_matching_data;
typedef struct{
	size_t index;
	const char* file;
} matched_font;
#endif

font_matching_data matcher_init();

matched_font match_font(
	font_matching_data matcher,
	const char* family,
	utf16 text,
	size_t* run_length
);

void close_font(matched_font font);

typedef struct {
	utf8 str;
	uint64_t hash;
	sg_image img;
	sg_view view;
	bool alive;
} rendered_str;

typedef struct font_run font_run;
struct font_run{
	FT_Face face;
	size_t len;
	font_run* next;
};

TextSize measure(
	FT_Library lib,
	utf8 str,
	FT_UInt font_size,
	font_run* faces
);
rendered_str* render(FT_Library lib, utf8 str, FT_UInt font_size);

#endif // _RJ_FONT_RENDERER
#ifdef RJ_FONT_RENDERER_IMPL
#undef RJ_FONT_RENDERER_IMPL

TextBitmap createTextBitmap(TextSize size){
	TextBitmap tb;
	tb.buf_len = (size_t)((size.width>>6)*(size.height>>6));
	tb.buf = (uint8_t*)calloc(tb.buf_len, 1);
	tb.size = size;
	return tb;
}
void TextBitmap_destroy(TextBitmap tb){
	free(tb.buf);
}

TextSize render_text(FT_Face face, utf8 text, TextBitmap* image, FT_Pos offset){
	FT_GlyphSlot slot = face->glyph;
	bool has_kerning = (bool)(face->face_flags & FT_FACE_FLAG_KERNING);
	
	/* set up matrix */
	FT_Matrix matrix;
	matrix.xx = 0x10000L;
	matrix.xy = 0;
	matrix.yx = 0;
	matrix.yy = 0x10000L;

	FT_Vector pen = {.x = offset, .y = 0};
	if (image){
		pen.y = image->size.baseline;
	}
	
	FT_Long min_pos = 0;
	FT_Long max_pos = 0;
	FT_ULong lastglyph;
	
	for (int n = 0; n < text.len;) {
		codepoint_return gr = codepoint_from_utf8((utf8){&text.chars[n], text.len-n});
		FT_ULong utf8char = gr.codepoint;
		n += gr.consumed;
		
		FT_UInt glyph = FT_Get_Char_Index(face, utf8char);
		
		if (n>0 && has_kerning){
			FT_Vector delta;

			FT_Get_Kerning(face, lastglyph, glyph, FT_KERNING_DEFAULT, &delta);
		
			pen.x += delta.x;
			pen.y += delta.y;
		}
		lastglyph = glyph;

		FT_Set_Transform(face, &matrix, &pen);
		
		FT_Error error = FT_Load_Glyph(face, glyph, image?FT_LOAD_RENDER:FT_LOAD_NO_BITMAP);
		assert(!error);
		
		if (image){
			int width = image->size.width>>6;
			int height = image->size.height>>6;
			FT_Bitmap* bitmap = &slot->bitmap;
			FT_Int x = slot->bitmap_left;
			FT_Int y = (FT_Int)(height - slot->bitmap_top);
			FT_Int i, j, p, q;
			FT_Int x_max = x + bitmap->width;
			FT_Int y_max = y + bitmap->rows;


			/* for simplicity, we assume that `bitmap->pixel_mode' */
			/* is `FT_PIXEL_MODE_GRAY' (i.e., not a bitmap font)   */

			// assert(bitmap->pixel_mode == FT_PIXEL_MODE_GRAY);

			for ( i = x, p = 0; i < x_max; i++, p++ ){
				for ( j = y, q = 0; j < y_max; j++, q++ ){
					if (i < 0 || j < 0 || i >= width || j >= height){ continue; }

					image->buf[width * j + i] |= bitmap->buffer[q * bitmap->width + p];
					// image->buf[width * j + i] = image->buf[width * j + i] * (
					// 	255 - bitmap->buffer[q * bitmap->width + p]
					// ) + bitmap->buffer[q * bitmap->width + p];
				}
			}
		}

		if (slot->metrics.horiBearingY > max_pos){max_pos = slot->metrics.horiBearingY;}
		if (slot->metrics.horiBearingY-slot->metrics.height < min_pos){min_pos = slot->metrics.horiBearingY-slot->metrics.height;}

		pen.x += slot->advance.x;
		pen.y += slot->advance.y;
	}

	return (TextSize){
		// FIXME: the end of the char is sometimes cut of on fancy fonts
		// using the width is slightly better than the advance as some fonts
		// dont advance their full width. however it can still somtimes cut
		// the end of the character on some fonts?

		// FIXME: in theory more chars cut of their ends?
		// also realy we should take a max of each char pen.x+metric.width
		// as it would be posible to have a character have a width greater
		// than its advance + the next characters width
		// (the fix to both these is likely the same?)
		// maybe somthing to do with FT_Glyph_Get_CBox?
		pen.x - offset - slot->advance.x + slot->metrics.width,
		(max_pos - min_pos),
		-min_pos
	};
}

#if defined(ANDROID)

font_matching_data matcher_init(){return AFontMatcher_create();}

matched_font match_font(
	font_matching_data matcher,
	const char* family,
	utf16 text,
	size_t* run_length
){
	AFont* font = AFontMatcher_match(
		matcher,
		"sans-serif",
		text.chars,
		text.len,
		(uint32_t*)run_length
	);

	return (matched_font){
		AFont_getCollectionIndex(font),
		AFont_getFontFilePath(font),
		font
	};
}

void close_font(matched_font font){
	AFont_close(font._font);
}

#elif defined(_WIN32)
font_matching_data matcher_init(){return NULL;}

matched_font match_font(
	font_matching_data matcher,
	const char* family,
	utf16 text,
	size_t* run_length
){
	// LOGFONT lf;
	// lf.lfCharSet = ANSI_CHARSET;


	// lf.lfCharSet = ANSI_CHARSET;

	// EnumFontFamiliesEx(
	// 	GetDC(NULL),

	// )

	*run_length = text.len;

	return (matched_font){
		0,
		"C:\\windows\\fonts\\arialbd.ttf"
	};

}

void close_font(matched_font font){

}

#endif

#define RENDERED_CACHE_SIZE 50

rendered_str rendered_cache[RENDERED_CACHE_SIZE] = {0};

/*FORCE_INLINE*/ TextSize addTextSize(TextSize lhs, TextSize rhs){
	TextSize rv;
	rv.width = lhs.width+rhs.width;
	rv.baseline = max(lhs.baseline, rhs.baseline);
	rv.height = max(
		lhs.height - lhs.baseline,
		rhs.height - rhs.baseline
	) + rv.baseline;
	return rv;
}

uint64_t hash(uint8_t* str, size_t len) {
	// http://www.cse.yorku.ca/%7Eoz/hash.html#sdbm
	uint64_t hash = 0;
	for (uint8_t* c = str; c < str+len; c++) {
		hash = *c + (hash << 6) + (hash << 16) - hash;
	}
	return hash;
}

TextSize measure(
	FT_Library lib,
	utf8 str,
	FT_UInt font_size,
	font_run* faces
) {
	assert(str.len > 0);
	utf16 text = {0};
	text.len = utf8_to_utf16(str, text);
	text.chars = malloc(text.len*sizeof(text.chars[0]));
	void* chars = text.chars;
	utf8_to_utf16(str, text);
	font_matching_data matcher = matcher_init();
	TextSize size = {0};
	font_run* face = faces;
	if (face == NULL){face = calloc(1, sizeof(*face));}
	while (true) {
		matched_font font = match_font(
			matcher,
			"sans-serif",
			text,
			&face->len
		);
		assert(face->len > 0);

		//TODO: cache fonts
		FT_Error err = FT_New_Face(lib, font.file, font.index, &face->face);
		assert(!err);

		err = FT_Set_Pixel_Sizes(face->face, font_size, font_size);
		assert(!err);
		size = addTextSize(size, render_text(face->face, str, NULL, 0));
		
		close_font(font);
		
		text.len -= face->len;
		text.chars += face->len;

		if (text.len <= 0){break;}

		if (face->next == NULL){face->next = calloc(1, sizeof(*face));}
		face = face->next;
	}
	// cleanup
	free(chars);
	if (faces == NULL){
		while (faces != NULL){
			font_run* next = faces->next;
			free(faces);
			faces = next;
		}
	}
	return size;
}
rendered_str* render(FT_Library lib, utf8 str, FT_UInt font_size) {
	rendered_str* rv = NULL;
	for (rendered_str* i = rendered_cache; i < rendered_cache+RENDERED_CACHE_SIZE; i++) {
		if (!i->alive){
			if (rv == NULL){rv = i;}
			continue;
		}
		if (
			i->str.chars == str.chars &&
			i->str.len == str.len &&
			i->hash == hash(str.chars, str.len)
		){
			i->alive = true;
			if (
				sg_query_view_state(i->view) == SG_RESOURCESTATE_VALID &&
				sg_query_image_state(i->img) == SG_RESOURCESTATE_VALID
			){return i;}
			sg_destroy_view(i->view);
			sg_destroy_image(i->img);

			rv = i;
			break;
		}
	}
	assert(rv != NULL && "CACHE FULL");
	// acctualy render the string
	font_run faces[2] = {0};
	faces[0].next = &faces[1];
	TextBitmap tb = createTextBitmap(measure(lib, str, font_size, faces));
	FT_Pos offset = 0;
	for(
		font_run* face = faces;
		face != NULL && face->len > 0;
		face = face->next
	){
		FT_Error err = FT_Set_Pixel_Sizes(face->face, font_size, font_size);
		assert(!err);
		offset += render_text(face->face, str, &tb, offset).width;
	}


	rv->img = sg_make_image(&(sg_image_desc){
		.width = tb.size.width>>6,
		.height = tb.size.height>>6,
		.pixel_format = SG_PIXELFORMAT_R8,
		.data.mip_levels[0] = {
			.ptr = tb.buf,
			.size = tb.buf_len,
		},
	});
	rv->view = sg_make_view(&(sg_view_desc){
		.texture.image = rv->img,
	});
	rv->str = str;
	rv->hash = hash(str.chars, str.len);
	rv->alive = true;

	TextBitmap_destroy(tb);

	font_run* face = faces;
	while (face != NULL){
		font_run* next = face->next;
		if (face < faces || ((uint8_t*)face) > ((uint8_t*)faces)+sizeof(faces)){
			free(face);
		}
		face = next;
	}

	return rv;
}
#endif // RJ_FONT_RENDERER_IMPL