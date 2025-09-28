#include <freetype/freetype.h>
#define RJ_STRINGS_IMPL
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

// fn void render_bitmap(Olivec_Canvas oc, TextBitmap text, usz baseline_left, usz baseline_y, Un32 colour){
// 	// TODO: dont ignore colour alpha
// 	Un32 fr = olive::olivec_red(colour);
// 	Un32 fg = olive::olivec_green(colour);
// 	Un32 fb = olive::olivec_blue(colour);
// 	Un32 fa = olive::olivec_alpha(colour);
// 	for (usz i = 0; i < text.size.height>>6; i++) {
// 		for (usz j = 0; j < text.size.width>>6; j++){
// 			isz x = j + baseline_left;
// 			isz y = i+baseline_y-(usz)(text.size.height-text.size.baseline)>>6;
// 			if (x<0 || x>=oc.width || y<0 || y>=oc.height){continue;}
// 			olive::olivec_blend_color(
// 				&oc.pixels[y*oc.stride + x],
// 				olive::olivec_rgba(fr, fg, fb, text.buf[i * (usz)(text.size.width>>6) + j])
// 			);
// 		}
// 	}
// }

TextSize render_text(FT_Face face, uint8_t* text, size_t text_len, TextBitmap* image, FT_Pos offset){
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
	
	for (int n = 0; n < text_len;) {
		FT_ULong utf8char;
		if (text[n] >> 3 == 0b11110){
			FT_ULong b1 = text[n];
			n++;
			FT_ULong b2 = text[n];
			n++;
			FT_ULong b3 = text[n];
			n++;
			FT_ULong b4 = text[n];
			utf8char = (b1&0b111)<<18 | (b2&0b111111)<<12 | (b3&0b111111)<<6 | (b4&0b111111);
		}else if (text[n] >> 4 == 0b1110){
			FT_ULong b1 = text[n];
			n++;
			FT_ULong b2 = text[n];
			n++;
			FT_ULong b3 = text[n];
			utf8char = (b1&0b1111)<<12 | (b2&0b111111)<<6 | (b3&0b111111);
		}else if (text[n] >> 5 == 0b110){
			FT_ULong b1 = text[n];
			n++;
			FT_ULong b2 = text[n];
			utf8char = (b1&0b11111)<<6 | (b2&0b111111);
		}else{
			utf8char = text[n];
		}
		n++;
		
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

#ifdef ANDROID
#include <android/font_matcher.h>
#define RENDERED_CACHE_SIZE 50


typedef struct {
	uint8_t* str;
	size_t len;
	uint64_t hash;
	sg_image img;
	sg_view view;
	bool alive;
} rendered_str;

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

typedef struct font_run font_run;
struct font_run{
	FT_Face face;
	uint32_t len;
	font_run* next;
};
TextSize android_measure(
	FT_Library lib,
	uint8_t* str,
	size_t len,
	FT_UInt font_size,
	font_run* faces
) {
	assert(len > 0);
	utf16 text = {0};
	text.len = utf8_to_utf16((utf8){str, len}, text);
	text.chars = malloc(text.len*sizeof(text.chars[0]));
	void* chars = text.chars;
	utf8_to_utf16((utf8){str, len}, text);
	AFontMatcher* matcher = AFontMatcher_create();
	assert(matcher);
	TextSize size = {0};
	font_run* face = faces;
	if (face == NULL){face = calloc(1, sizeof(*face));}
	while (true) {
		AFont* font = AFontMatcher_match(
			matcher,
			"sans-serif",
			text.chars,
			text.len,
			&face->len
		);
		assert(face->len > 0);
		size_t font_index = AFont_getCollectionIndex(font);
		const char* font_file = AFont_getFontFilePath(font);
		
		//TODO: cache fonts
		FT_Error err = FT_New_Face(lib, font_file, font_index, &face->face);
		assert(!err);
		
		err = FT_Set_Pixel_Sizes(face->face, font_size, font_size);
		assert(!err);
		size = addTextSize(size, render_text(face->face, str, len, NULL, 0));
		
		AFont_close(font);
		
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
rendered_str* android_render(FT_Library lib, uint8_t* str, size_t len, FT_UInt font_size) {
	rendered_str* rv = NULL;
	for (rendered_str* i = rendered_cache; i < rendered_cache+RENDERED_CACHE_SIZE; i++) {
		if (!i->alive){
			if (rv == NULL){rv = i;}
			continue;
		}
		if (i->str == str && i->len == len && i->hash == hash(str, len)){
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
	TextBitmap tb = createTextBitmap(android_measure(lib, str, len, font_size, faces));
	FT_Pos offset = 0;
	for(
		font_run* face = faces;
		face != NULL && face->len > 0;
		face = face->next
	){
		FT_Error err = FT_Set_Pixel_Sizes(face->face, font_size, font_size);
		assert(!err);
		offset += render_text(face->face, str, len, &tb, offset).width;
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
	rv->len = len;
	rv->hash = hash(str, len);
	rv->alive = true;

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

#endif
