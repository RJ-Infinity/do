#include <stdio.h>
#include <stdlib.h>
#define CLAY_IMPLEMENTATION
#include <clay.h>
#include "layout.h"
#include <assert.h>
#include "log.h"


// not sure what renderer i will use with other systems so include them in android only for now
#if defined(ANDROID)
	// these must be included before sokol
	#include <EGL/egl.h>
	#include <glad/glad.h>

	#define SOKOL_GLES3
#elif defined(_WIN32)
	#define SOKOL_GLCORE
#endif
#define SOKOL_GFX_IMPL
#include <sokol_gfx.h>

#define SOKOL_GL_IMPL
#include <sokol_gl.h>

#define SOKOL_CLAY_NO_SOKOL_APP
#define SOKOL_CLAY_IMPL
#include <sokol_clay.h>

#define RJ_STRINGS_IMPL
#include "unicode.h"
#define RJ_FONT_RENDERER_IMPL
#include "font_renderer.h"
#include <shader/text.h>

void panic(const char* fmt, ...);

#if defined(ANDROID)
#include <android_native_app_glue.h>
#include <android/set_abort_message.h>
#include <android/choreographer.h>
#include <android/window.h>
#include <android/font_matcher.h>
#include <jni.h>

typedef struct android_app android_app;
typedef struct android_poll_source android_poll_source;

#define user_rd(app) ((rendering_data*)((app)->userData))
#elif defined(_WIN32)
#include <windows.h>
#endif


sg_sampler txt_smp;
sgl_pipeline txt_pip;
void sclay_render_text(Clay_RenderCommand *config, sclay_font_context ctx){
	rendered_str* rs = render(
		(FT_Library)ctx,
		(utf8){
			(u8*)config->renderData.text.stringContents.chars,
			config->renderData.text.stringContents.length,
		},
		config->renderData.text.fontSize
	);

	if (txt_smp.id == 0 || sg_query_sampler_state(txt_smp) != SG_RESOURCESTATE_VALID){
		txt_smp = sg_make_sampler(&(sg_sampler_desc){
			.min_filter = SG_FILTER_LINEAR,
			.mag_filter = SG_FILTER_LINEAR,
			.wrap_u = SG_WRAP_CLAMP_TO_EDGE,
			.wrap_v = SG_WRAP_CLAMP_TO_EDGE,
		});
	}
	if (txt_pip.id == 0){// || sg_query_pipeline_state((sg_pipeline){txt_pip.id}) != SG_RESOURCESTATE_VALID){
		txt_pip = sgl_make_pipeline(&(sg_pipeline_desc){
			.shader = sg_make_shader(sgl_shader_desc(sg_query_backend())),
			.colors[0] = {.blend = {
				.enabled = true,
				.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
				.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
				.op_rgb = SG_BLENDOP_ADD,
				.src_factor_alpha = SG_BLENDFACTOR_ONE,
				.dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
				.op_alpha = SG_BLENDOP_ADD,
			}},
		});
	}

	sgl_push_pipeline();
	sgl_load_pipeline(txt_pip);

	sgl_c4f(
		((float)config->renderData.text.textColor.r) / 255.f,
		((float)config->renderData.text.textColor.g) / 255.f,
		((float)config->renderData.text.textColor.b) / 255.f,
		((float)config->renderData.text.textColor.a) / 255.f
	);
	sgl_texture(rs->view, txt_smp);
	sgl_enable_texture();

	sgl_begin_quads();
	sgl_v2f_t2f(config->boundingBox.x, config->boundingBox.y, 0.0f, 0.0f);
	sgl_v2f_t2f(config->boundingBox.x + config->boundingBox.width, config->boundingBox.y, 1.0f, 0.0f);
	sgl_v2f_t2f(config->boundingBox.x + config->boundingBox.width, config->boundingBox.y + config->boundingBox.height, 1.0f, 1.0f);
	sgl_v2f_t2f(config->boundingBox.x, config->boundingBox.y + config->boundingBox.height, 0.0f, 1.0f);

	// sg_apply_bindings(&(sg_bindings){
	// 	.views[0] = rs->view
	// });

	sgl_end();
	sgl_disable_texture();


	sgl_pop_pipeline();
}


Clay_Dimensions measureText(
	Clay_StringSlice text,
	Clay_TextElementConfig *config,
	void *userData
){
	TextSize ts = measure(
		(FT_Library)userData,
		(utf8){
			(u8*)text.chars,
			text.length,
		},
		config->fontSize,
		NULL
	);
	return (Clay_Dimensions){
		.width = ts.width >> 6,
		.height = ts.height >> 6,
	};
}


void handle_clay_error(Clay_ErrorData errorText){
	switch (errorText.errorType){
		case CLAY_ERROR_TYPE_TEXT_MEASUREMENT_FUNCTION_NOT_PROVIDED:{
			LOG_W("CLAY_ERROR_TYPE_TEXT_MEASUREMENT_FUNCTION_NOT_PROVIDED thrown");
		}break;
		case CLAY_ERROR_TYPE_ARENA_CAPACITY_EXCEEDED:{
			LOG_W("CLAY_ERROR_TYPE_ARENA_CAPACITY_EXCEEDED thrown");
		}break;
		case CLAY_ERROR_TYPE_ELEMENTS_CAPACITY_EXCEEDED:{
			LOG_W("CLAY_ERROR_TYPE_ELEMENTS_CAPACITY_EXCEEDED thrown");
		}break;
		case CLAY_ERROR_TYPE_TEXT_MEASUREMENT_CAPACITY_EXCEEDED:{
			LOG_W("CLAY_ERROR_TYPE_TEXT_MEASUREMENT_CAPACITY_EXCEEDED thrown");
		}break;
		case CLAY_ERROR_TYPE_DUPLICATE_ID:{
			LOG_W("CLAY_ERROR_TYPE_DUPLICATE_ID thrown");
		}break;
		case CLAY_ERROR_TYPE_FLOATING_CONTAINER_PARENT_NOT_FOUND:{
			LOG_W("CLAY_ERROR_TYPE_FLOATING_CONTAINER_PARENT_NOT_FOUND thrown");
		}break;
		case CLAY_ERROR_TYPE_PERCENTAGE_OVER_1:{
			LOG_W("CLAY_ERROR_TYPE_PERCENTAGE_OVER_1 thrown");
		}break;
		case CLAY_ERROR_TYPE_INTERNAL_ERROR:{
			LOG_W("CLAY_ERROR_TYPE_INTERNAL_ERROR thrown");
		}break;
	}
	panic("unhandlable clay error: %.*s", errorText.errorText.length, errorText.errorText.chars);
}

void sokol_log(
	const char* tag,                // always "sg"
	uint32_t log_level,             // 0=panic, 1=error, 2=warning, 3=info
	uint32_t log_item_id,           // SG_LOGITEM_*
	const char* message_or_null,    // a message string, may be nullptr in release mode
	uint32_t line_nr,               // line number in sokol_gfx.h
	const char* filename_or_null,   // source filename, may be nullptr in release mode
	void* user_data
){
	if (message_or_null){
		switch (log_level){
			case 0: LOG_F("sokol log `%s` %s:%d", message_or_null, filename_or_null, line_nr); break;
			case 1: LOG_E("sokol log `%s` %s:%d", message_or_null, filename_or_null, line_nr); break;
			case 2: LOG_W("sokol log `%s` %s:%d", message_or_null, filename_or_null, line_nr); break;
			case 3:
			default: LOG_I("sokol log `%s` %s:%d", message_or_null, filename_or_null, line_nr); break;
		}
	}else{
		switch (log_level){
			case 0: LOG_F("sokol empty log"); break;
			case 1: LOG_E("sokol empty log"); break;
			case 2: LOG_W("sokol empty log"); break;
			case 3:
			default: LOG_I("sokol empty log"); break;
		}
	}
}


#ifdef ANDROID
typedef struct{
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	int framebuffer;
	int32_t width;
	int32_t height;
	float dpi;
	ARect contentRect;
	bool running;
	float pointer_x;
	float pointer_y;
	bool pointer;
	FT_Library library;
} rendering_data;

void panic(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	char* buf;
	if (vasprintf(&buf, fmt, ap) < 0) {
		android_set_abort_message("failed for format error message");
	} else {
		android_set_abort_message(buf);
		LOG_E("PANIC: %s", buf);
	}
	exit(1);
}

bool init_graphics(android_app* app){
	// initialise EGL this is mostly taken from https://github.com/floooh/sokol/pull/87/files
	printf("setting up egl");
	EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (display == EGL_NO_DISPLAY) {
		LOG_W("Unable to get default display");
		return false;
	}
	if (eglInitialize(display, NULL, NULL) == EGL_FALSE) {
		LOG_W("Unable to initialise EGL");
		return false;
	}
	const EGLint cfg_attributes[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_DEPTH_SIZE, 16,
		EGL_STENCIL_SIZE, 0,
		EGL_NONE,
	};
	
	EGLint cfg_count;
	eglChooseConfig(display, cfg_attributes, NULL, 0, &cfg_count);
	EGLConfig available_cfgs[cfg_count];
	assert(available_cfgs);
	eglChooseConfig(display, cfg_attributes, available_cfgs, cfg_count, &cfg_count);
	assert(cfg_count);

	EGLConfig config;
	bool exact_cfg_found = false;
	for (int i = 0; i < cfg_count; ++i) {
		EGLConfig c = available_cfgs[i];
		EGLint r, g, b, a, d;
		if (
			eglGetConfigAttrib(display, c, EGL_RED_SIZE, &r) == EGL_TRUE &&
			eglGetConfigAttrib(display, c, EGL_GREEN_SIZE, &g) == EGL_TRUE &&
			eglGetConfigAttrib(display, c, EGL_BLUE_SIZE, &b) == EGL_TRUE &&
			eglGetConfigAttrib(display, c, EGL_ALPHA_SIZE, &a) == EGL_TRUE &&
			eglGetConfigAttrib(display, c, EGL_DEPTH_SIZE, &d) == EGL_TRUE &&
			r == 8 && g == 8 && b == 8 && a == 8 && d == 16
		) {
			exact_cfg_found = true;
			config = c;
			break;
		}
	}
	if (!exact_cfg_found) {
		LOG_W("exact config not found?");
		config = available_cfgs[0];
	}

	EGLint ctx_attributes[] = {
		EGL_CONTEXT_MAJOR_VERSION, 3,
		EGL_CONTEXT_MINOR_VERSION, 1,
		EGL_NONE,
	};

	EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, ctx_attributes);
	if (context == EGL_NO_CONTEXT) {
		LOG_W("failed to create EGL context");
		return false;
	}

	EGLSurface surface = eglCreateWindowSurface(display, config, app->window, NULL);
	if (surface == EGL_NO_SURFACE) {
		LOG_W("failed to create window surface");
		return false;
	}
	if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
		LOG_W("eglMakeCurrent failed");
		return false;
	}

	eglQuerySurface(display, surface, EGL_WIDTH, &user_rd(app)->width);
	eglQuerySurface(display, surface, EGL_HEIGHT, &user_rd(app)->height);
	
	user_rd(app)->display = display;
	user_rd(app)->context = context;
	user_rd(app)->surface = surface;
	// TODO: probably a better way of acsessing this
	user_rd(app)->contentRect = app->contentRect;
	user_rd(app)->dpi = (float)user_rd(app)->width / (float)ANativeWindow_getWidth(app->window);
	if (user_rd(app)->dpi < 0){
		LOG_W("failed to get window size");
		return false;
	}
	
	printf("load gl functions");
	// load GL functions
	if (!gladLoadGLES2Loader((GLADloadproc)eglGetProcAddress)) {
		LOG_W("failed to load GL with glad");
		return false;
	}
	
	LOG_I("OpenGL Info: %s", glGetString(GL_VENDOR));
	LOG_I("OpenGL Info: %s", glGetString(GL_RENDERER));
	LOG_I("OpenGL Info: %s", glGetString(GL_VERSION));
	LOG_I("OpenGL Info: %s", glGetString(GL_EXTENSIONS));

	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &user_rd(app)->framebuffer);
	printf("framebuffer was %d", user_rd(app)->framebuffer);


	printf("setup sokol gfx");
	// setup sokol gfx
	
	
	sg_desc sokol_desc = {0};
	
	sokol_desc.logger.func = sokol_log;
	sokol_desc.environment.defaults.color_format = SG_PIXELFORMAT_RGBA8;
	sokol_desc.environment.defaults.depth_format = SG_PIXELFORMAT_DEPTH_STENCIL;
	sokol_desc.environment.defaults.sample_count = 1;
	sg_setup(&sokol_desc);
	
	printf("setup sgl");

	sgl_desc_t sgl_desc = {0};
	sgl_desc.color_format = SG_PIXELFORMAT_RGBA8;
	sgl_desc.depth_format = SG_PIXELFORMAT_DEPTH_STENCIL;
	sgl_desc.sample_count = 1;

	sgl_setup(&sgl_desc);

	printf("setup sclay");
	sclay_setup();

	return true;
}
static void deinit_graphics(rendering_data* rd) {
	sclay_shutdown();
	sgl_shutdown();
	sg_shutdown();

	// same source as init EGL
	if (rd->display != EGL_NO_DISPLAY) {
		eglMakeCurrent(rd->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		if (rd->surface != EGL_NO_SURFACE) {
			LOG_I("Destroying egl surface");
			eglDestroySurface(rd->display, rd->surface);
			rd->surface = EGL_NO_SURFACE;
		}
		if (rd->context != EGL_NO_CONTEXT) {
			LOG_I("Destroying egl context");
			eglDestroyContext(rd->display, rd->context);
			rd->context = EGL_NO_CONTEXT;
		}
		LOG_I("Terminating egl display");
		eglTerminate(rd->display);
		rd->display = EGL_NO_DISPLAY;
	}
}

void tick(rendering_data* rd);
void do_tick(long data, void* rd){
	tick(rd);
}

void tick(rendering_data* rd) {
	if (!rd->running){return;}
	AChoreographer_postFrameCallback64(AChoreographer_getInstance(), do_tick, rd);

	if (rd->display == NULL) {
		return;
	}

	sclay_set_layout_dimensions((Clay_Dimensions){(float)rd->width, (float)rd->height}, rd->dpi);
	Clay_SetPointerState((Clay_Vector2){rd->pointer_x, rd->pointer_y}, rd->pointer);

	Clay_RenderCommandArray commands = layout((Clay_Padding){
		.left = rd->contentRect.left,
		.top = rd->contentRect.top,
		.right = rd->width - rd->contentRect.right,
		.bottom = rd->height - rd->contentRect.bottom,
	}, get_things());

	sg_begin_pass(&(sg_pass){.swapchain = (sg_swapchain){
		.width = rd->width,
		.height = rd->height,
		.sample_count = 1,
		.color_format = SG_PIXELFORMAT_RGBA8,
		.depth_format = SG_PIXELFORMAT_DEPTH_STENCIL,
		.gl.framebuffer = rd->framebuffer,
	}});
	sgl_matrix_mode_modelview();
	sgl_load_identity();

	sclay_render(commands);

	sgl_draw();
	sg_end_pass();
	sg_commit();
	
	if (!eglSwapBuffers(rd->display, rd->surface)) {
		EGLint err = eglGetError();
		printf("eglSwapBuffers failed: 0x%x\n", err);
	}

}

void android_handle_cmd(android_app* app, int32_t cmd) {
	switch (cmd) {
		case APP_CMD_SAVE_STATE:{
			printf("save state");
			// app->savedState = malloc(sizeof(SavedState));
			// *((SavedState*)app->savedState) = state;
			// app->savedStateSize = sizeof(SavedState);
		}break;
		case APP_CMD_INIT_WINDOW:{
			// The window is being shown, get it ready.
			if (app->window != NULL) {
				printf("init window");
				init_graphics(app);
			}
		}break;
		case APP_CMD_TERM_WINDOW:{
			// The window is being hidden or closed, clean it up.
			printf("terminate window");
			deinit_graphics(user_rd(app));
		}break;
		case APP_CMD_GAINED_FOCUS:{
			printf("gained focus");
			user_rd(app)->running = true;
			tick(user_rd(app));
		}break;
		case APP_CMD_LOST_FOCUS:{
			printf("lost focus");
			user_rd(app)->running = false;
		}break;
		default:break;
	}
}
int32_t android_handle_input(android_app* app, AInputEvent* event) {
	switch (AInputEvent_getType(event)) {
		case AINPUT_EVENT_TYPE_KEY:{
			printf("AINPUT_EVENT_TYPE_KEY");
		}break;
		case AINPUT_EVENT_TYPE_MOTION:{
			user_rd(app)->pointer_x = AMotionEvent_getX(event, 0);
			user_rd(app)->pointer_y = AMotionEvent_getY(event, 0);
			int32_t action = AMotionEvent_getAction(event);
			user_rd(app)->pointer = (
				action == AMOTION_EVENT_ACTION_DOWN ||
				action == AMOTION_EVENT_ACTION_MOVE
			);
		}break;
		case AINPUT_EVENT_TYPE_FOCUS:{
			printf("AINPUT_EVENT_TYPE_FOCUS");
		}break;
		case AINPUT_EVENT_TYPE_CAPTURE:{
			printf("AINPUT_EVENT_TYPE_CAPTURE");
		}break;
		case AINPUT_EVENT_TYPE_DRAG:{
			printf("AINPUT_EVENT_TYPE_DRAG");
		}break;
		case AINPUT_EVENT_TYPE_TOUCH_MODE:{
			printf("AINPUT_EVENT_TYPE_TOUCH_MODE");
		}break;
	}
	return 0;
}

// void set_status_bar_colour(android_app* app){
// 	JNIEnv* env;
// 	(*app->activity->vm)->AttachCurrentThread(app->activity->vm, &env, NULL);

// 	jobject activity_obj = app->activity->clazz;
// 	jclass activity_class = (*env)->GetObjectClass(env, activity_obj);

// 	jmethodID mid_getWindow = (*env)->GetMethodID(
// 		env,
// 		activity_class,
// 		"getWindow",
// 		"()Landroid/view/Window;"
// 	);

// 	jobject window_obj = (*env)->CallObjectMethod(env, activity_obj, mid_getWindow);
// 	jclass window_class = (*env)->GetObjectClass(env, window_obj);

// 	jmethodID mid_getDecorView = (*env)->GetMethodID(
// 		env,
// 		window_class,
// 		"getDecorView",
// 		"()Landroid/view/View;"
// 	);

// 	jobject view_obj = (*env)->CallObjectMethod(env, window_obj, mid_getDecorView);
// 	jclass view_class = (*env)->GetObjectClass(env, view_obj);

// 	jmethodID mid_setForceDarkAllowed = (*env)->GetMethodID(
// 		env,
// 		view_class,
// 		"setForceDarkAllowed",
// 		"(Z)"
// 	);
// 	(*env)->CallObjectMethod(env, view_obj, mid_setForceDarkAllowed, JNI_FALSE);

// 	(*app->activity->vm)->DetachCurrentThread(app->activity->vm);
// }

void android_main(android_app* app) {
	rendering_data rd = {0};
	app->userData = &rd;
	app->onAppCmd = android_handle_cmd;
	app->onInputEvent = android_handle_input;

	// set_status_bar_colour(app);

	FT_Error error = FT_Init_FreeType(&rd.library);
	assert(!error);

	size_t clayMemReq = Clay_MinMemorySize();
	Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(clayMemReq, malloc(clayMemReq));

	Clay_Initialize(arena, (Clay_Dimensions) { 0, 0 }, (Clay_ErrorHandler){handle_clay_error});
	
	Clay_SetMeasureTextFunction(measureText, rd.library);
	sclay_set_font_ctx(rd.library);

	if (app->savedState != NULL) {
		printf("saved state %p", app->savedState);
	}

	while (!app->destroyRequested) {
		android_poll_source* source = NULL;
		int result = ALooper_pollOnce(-1, NULL, NULL, (void**)&source);
		if (result == ALOOPER_POLL_ERROR) {
			panic("ALooper_pollOnce returned an error");
		}

		if (source != NULL) {
			source->process(app, source);
		}
	}

	deinit_graphics(app->userData);
	FT_Done_FreeType(rd.library);
}
#elif defined(_WIN32)

typedef struct {
	HWND hwnd;
	HDC device_ctx;
	HGLRC GL_ctx;
	GLint framebuffer;
	FT_Library library;
} rendering_data;

void panic(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	size_t len = vsprintf(NULL, fmt, ap);
	char* buf = malloc(len);

	if (len <= 0 || buf == NULL || vsprintf(buf, fmt, ap) <= 0) {
		LOG_F("failed for format error message");
	} else {
		LOG_F("PANIC: %s", buf);
	}
	exit(1);
}

LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	// get the user data
	// GetWindowLongPtrW(hwnd, GWLP_USERDATA);

	switch (uMsg) {
		case WM_CLOSE: {
			DestroyWindow(hwnd);
		} break;
		case WM_DESTROY: {
			PostQuitMessage(0);
		} break;
		case WM_SIZE: {
			int width = LOWORD(lParam);
			int height = HIWORD(lParam);
		} break;
		case WM_PAINT: {

		} break;
		default: {
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
		}
	}
	return 0;
}

void tick(HWND hwnd){
	rendering_data* rd = (rendering_data*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
	if (rd == NULL){return;}
	RECT size;
	POINT mouse;
	assert(GetClientRect(hwnd, &size));
	assert(GetCursorPos(&mouse));

	if (
		size.right - size.left == 0 ||
		size.bottom - size.top == 0
	){return;}

	sclay_set_layout_dimensions((Clay_Dimensions){
		(float)(size.right - size.left),
		(float)(size.bottom - size.top)
	}, 1);
	// }, GetDpiForWindow(hwnd));
	Clay_SetPointerState((Clay_Vector2){
		(float)(mouse.x - size.left),
		(float)(mouse.x - size.right),
	}, GetAsyncKeyState(VK_LBUTTON) & 0x01);

	Clay_RenderCommandArray commands = layout((Clay_Padding){0}, get_things());

	sg_begin_pass(&(sg_pass){.swapchain = (sg_swapchain){
		.width = size.right - size.left,
		.height = size.bottom - size.top,
		.sample_count = 1,
		.color_format = SG_PIXELFORMAT_RGBA8,
		.depth_format = SG_PIXELFORMAT_DEPTH_STENCIL,
		.gl.framebuffer = rd->framebuffer,
	}});
	sgl_matrix_mode_modelview();
	sgl_load_identity();

	sclay_render(commands);

	sgl_draw();
	sg_end_pass();
	sg_commit();

	SwapBuffers(rd->device_ctx);
}

int main(){
	WNDCLASSEX wc;
	wc.cbSize = sizeof(wc);
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = WindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = GetModuleHandle(NULL);
	wc.hIcon = LoadIcon(NULL, (LPCWSTR)32512);
	wc.hCursor = LoadCursor(NULL, (LPCWSTR)32512);
	wc.hbrBackground = (HBRUSH)1;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = L"DO";
	wc.hIconSm = LoadIcon(NULL, (LPCWSTR)32512);

	if(!RegisterClassEx(&wc)){
		MessageBox(NULL, L"Window Registration Failed!", L"Error!", 0);
		return 1;
	}

	HWND hwnd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		L"DO",
		L"DO",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 240, 120,
		NULL, // no parent
		NULL, // no menu
		GetModuleHandle(NULL),
		NULL
	);
	
	if(hwnd == NULL) {
		MessageBox(NULL, L"Window Creation Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
		return 1;
	}
	
	rendering_data rd;
	rd.hwnd = hwnd;
	rd.device_ctx = GetDC(hwnd);

	PIXELFORMATDESCRIPTOR pfd = {
		.nSize = sizeof(PIXELFORMATDESCRIPTOR),
		.nVersion = 1,
		.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		.iPixelType = PFD_TYPE_RGBA,
		.cColorBits = 24,
		.cDepthBits = 24,
		.cStencilBits = 8,
		.iLayerType = PFD_MAIN_PLANE,
	};
	int pfn = ChoosePixelFormat(rd.device_ctx, &pfd);
	assert(pfn);
	SetPixelFormat(rd.device_ctx, pfn, &pfd);

	rd.GL_ctx = wglCreateContext(rd.device_ctx);
	wglMakeCurrent(rd.device_ctx, rd.GL_ctx);

	HGLRC (*wglCreateContextAttribsARB)(
		HDC hDC,
		HGLRC hShareContext,
		const int *attribList
	) = (void*)wglGetProcAddress("wglCreateContextAttribsARB");
	BOOL (*wglChoosePixelFormatARB)(
		HDC hdc,
		const int *piAttribIList,
		const FLOAT *pfAttribFList,
		UINT nMaxFormats,
		int *piFormats,
		UINT *nNumFormats
	) = (void*)wglGetProcAddress("wglChoosePixelFormatARB");
	assert(wglCreateContextAttribsARB && wglChoosePixelFormatARB);

	wglMakeCurrent(rd.device_ctx, NULL);
	wglDeleteContext(rd.GL_ctx);

	const int attribList[] = {
		0x2001/*WGL_DRAW_TO_WINDOW_ARB*/, GL_TRUE,
		0x2010/*WGL_SUPPORT_OPENGL_ARB*/, GL_TRUE,
		0x2011/*WGL_DOUBLE_BUFFER_ARB*/, GL_TRUE,
		0x2013/*WGL_PIXEL_TYPE_ARB*/, 0x202B/*WGL_TYPE_RGBA_ARB*/,
		0x2014/*WGL_COLOR_BITS_ARB*/, 32,
		0x2022/*WGL_DEPTH_BITS_ARB*/, 24,
		0x2023/*WGL_STENCIL_BITS_ARB*/, 8,
		0, // End
	};

	UINT numFormats;
	wglChoosePixelFormatARB(rd.device_ctx, attribList, NULL, 1, &pfn, &numFormats);

	DescribePixelFormat(rd.device_ctx, pfn, sizeof(pfd), &pfd);
	SetPixelFormat(rd.device_ctx, pfn, &pfd);

	int iContextAttribs[] = {
		0x2091/*WGL_CONTEXT_MAJOR_VERSION_ARB*/, 3,
		0x2092/*WGL_CONTEXT_MINOR_VERSION_ARB*/, 1,
		0x2094/*WGL_CONTEXT_FLAGS_ARB*/, 0x00000002/*WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB*/,
		0
	};

	rd.GL_ctx = wglCreateContextAttribsARB(rd.device_ctx, 0, iContextAttribs);
	wglMakeCurrent(rd.device_ctx, rd.GL_ctx);




	FT_Error error = FT_Init_FreeType(&rd.library);
	assert(!error);

	size_t clayMemReq = Clay_MinMemorySize();
	Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(clayMemReq, malloc(clayMemReq));

	Clay_Initialize(arena, (Clay_Dimensions) { 0, 0 }, (Clay_ErrorHandler){handle_clay_error});
	
	Clay_SetMeasureTextFunction(measureText, rd.library);
	sclay_set_font_ctx(rd.library);




	sg_desc sokol_desc = {0};
	
	sokol_desc.logger.func = sokol_log;
	sokol_desc.environment.defaults.color_format = SG_PIXELFORMAT_RGBA8;
	sokol_desc.environment.defaults.depth_format = SG_PIXELFORMAT_DEPTH_STENCIL;
	sokol_desc.environment.defaults.sample_count = 1;
	sg_setup(&sokol_desc);
	
	printf("setup sgl\n");

	sgl_desc_t sgl_desc = {0};
	sgl_desc.color_format = SG_PIXELFORMAT_RGBA8;
	sgl_desc.depth_format = SG_PIXELFORMAT_DEPTH_STENCIL;
	sgl_desc.sample_count = 1;

	sgl_setup(&sgl_desc);

	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &rd.framebuffer);

	printf("setup sclay\n");
	sclay_setup();

	// set some custom userdata
	SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&rd);

	printf("show window\n");
	//TODO: make this an option
	ShowWindow(hwnd, SW_MAXIMIZE);
	// ShowWindow(hwnd, 1);
	UpdateWindow(hwnd);
	
	printf("start loop\n");
	MSG msg;
	while (true){
		if (GetMessage(&msg, NULL, 0, 0) == -1){
			MessageBox(NULL, L"Get Message Failed", L"Error!", MB_ICONEXCLAMATION | MB_OK);
			break;
		}
		if (msg.message == WM_QUIT){ break; }
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		
		tick(hwnd);
	}

	wglMakeCurrent(rd.device_ctx, NULL);
	wglDeleteContext(rd.GL_ctx);

	return 0;
}
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd){return main();}
#endif
