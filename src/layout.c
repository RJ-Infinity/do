#include "layout.h"
#include "log.h"
#define SOKOL_CLAY_NO_SOKOL_APP
#include <sokol_gfx.h>
#include <sokol_clay.h>

void render_thing(thing* thing){
	CLAY({
		.layout = {
			.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0)},
			.padding = CLAY_PADDING_ALL(8),
		},
		.backgroundColor = {250, 250, 170, 255},
	}){
		CLAY_TEXT(((Clay_String){
			.isStaticallyAllocated = true,
			.length = thing->name.len,
			.chars = (const char *)thing->name.chars,
		}), CLAY_TEXT_CONFIG({.fontSize = 24, .textColor = {0, 0, 0, 255}}));
		CLAY({.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0)}}}){}
		// TODO: CREATE VECTOR LIB
		// CLAY({
		// 	.layout = { .height = CLAY_SIZING_GROW(0) },
		// 	.aspectRatio = 1,
		// 	.image = {}
		// }){}
		CLAY_TEXT(((Clay_String){
			.isStaticallyAllocated = true,
			.length = 1,
			.chars = "+",
		}), CLAY_TEXT_CONFIG({.fontSize = 24, .textColor = {0, 0, 0, 255}}));
	}
	if (thing->child_len > 0){
		CLAY({
			.layout = {
				.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0)},
				.padding = {32, 0, 8, 8},
				.childGap = 8,
				.layoutDirection = CLAY_TOP_TO_BOTTOM,
			}
		}){
			for (int i = 0; i < thing->child_len; i++){
				render_thing(&thing->children[i]);
			}
		}
	}
}

Clay_RenderCommandArray layout(Clay_Padding window_padding, thing* things){
	Clay_BeginLayout();

	window_padding.left += 16;
	window_padding.top += 16;
	window_padding.bottom += 16;
	window_padding.right += 16;

	// An example of laying out a UI with a fixed width sidebar and flexible width main content
	CLAY({
		.id = CLAY_ID("OuterContainer"),
		.layout = {
			.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)},
			.padding = window_padding,
			.layoutDirection = CLAY_TOP_TO_BOTTOM,
		},
		.backgroundColor = {250,250,255,255}
	}) {
		render_thing(things);
	}

	return Clay_EndLayout();
}
