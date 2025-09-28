#include "layout.h"
#include "log.h"
#define SOKOL_CLAY_NO_SOKOL_APP
#include <sokol_gfx.h>
#include <sokol_clay.h>
#define SWAP __builtin_bswap32

uint32_t _test_img_data[] = {
	SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), 
	SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), 
	SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), 
	SWAP(0x00000000), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), 
	SWAP(0x00000000), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), 
	SWAP(0x00000000), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), 
	SWAP(0x00000000), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), 
	SWAP(0x00000000), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), 
	SWAP(0x00000000), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), 
	SWAP(0x00000000), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), SWAP(0xFF0000FF), 
};
sclay_image test_img = {0};

Clay_RenderCommandArray layout(Clay_Padding window_padding){
	if (
		test_img.view.id == SG_INVALID_ID ||
		sg_query_view_state(test_img.view) == SG_RESOURCESTATE_INVALID
	){
		test_img.view = sg_make_view(&(sg_view_desc){
			.texture.image = sg_make_image(&(sg_image_desc){
				.width = 10,
				.height = 10,
				.pixel_format = SG_PIXELFORMAT_RGBA8,
				.data.mip_levels[0] = {
					.ptr = _test_img_data,
					.size = 10 * 10 * 4,
				},
			}),
		});
	}
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
			.childGap = 16
		},
		.backgroundColor = {250,250,255,255}
	}) {
		CLAY({
			.id = CLAY_ID("SideBar"),
			.layout = {
				.layoutDirection = CLAY_TOP_TO_BOTTOM,
				.sizing = {.width = CLAY_SIZING_FIXED(300), .height = CLAY_SIZING_GROW(0)},
				.padding = CLAY_PADDING_ALL(16),
				.childGap = 16
			},
			.backgroundColor = {224, 215, 210, 255}
		}) {
			CLAY({
				.id = CLAY_ID("ProfilePictureOuter"),
				.layout = {
					.sizing = {.width = CLAY_SIZING_GROW(0)},
					.padding = CLAY_PADDING_ALL(16),
					.childGap = 16,
					.childAlignment = {.y = CLAY_ALIGN_Y_CENTER}
				},
				.backgroundColor = {168, 66, 28, 255}
			}) {
				CLAY({
					.id = CLAY_ID("ProfilePicture"),
					.layout = {.sizing = {.width = CLAY_SIZING_FIXED(60), .height = CLAY_SIZING_FIXED(60)}},
					.image = { .imageData = &test_img },
					.backgroundColor = {255, 255, 255, 255},
				}) {}
				CLAY_TEXT(CLAY_STRING("Clay - UI Library"), CLAY_TEXT_CONFIG({ .fontSize = 24, .textColor = {255, 255, 255, 255} }));
			}

			// // Standard C code like loops etc work inside components
			// for (int i = 0; i < 5; i++) {
			// 	SidebarItemComponent();
			// }

			CLAY({
				.id = CLAY_ID("MainContent"),
				.layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}},
				.backgroundColor = {224, 215, 210, 255}
			}) {}
		}
	}

	// CLAY({
	// 	.id = CLAY_ID("TEST"),
	// 	.layout = {.sizing = {.width = CLAY_SIZING_FIXED(100), .height = CLAY_SIZING_FIXED(100)}},
	// 	.backgroundColor = {10, 224, 40, 255},
	// }){}

	// All clay layouts are declared between Clay_BeginLayout and Clay_EndLayout
	return Clay_EndLayout();
}
