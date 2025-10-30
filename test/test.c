#include <stdio.h>
#include <stdbool.h>
#define EXPORT __declspec(dllexport)

#include <unicode.h>
typedef void(*assert_fn)(u8* condition, u8* reason, int line, u8* file);
assert_fn _tassert;
EXPORT void set_assert(assert_fn assert){_tassert = assert;}

#define RJ_ASSERT(condition) (void)( \
	(!!(condition)) || \
	( \
		_tassert((u8*)(#condition), (u8*)"unicode error", __LINE__, (u8*)__FILE__), \
		__builtin_trap(), \
		0 \
	) \
)
#define RJ_STRINGS_IMPL
#include <unicode.h>
#undef assert

#define assert(condition, reason) (void)( \
	(!!(condition)) || \
	( \
		_tassert((u8*)(#condition), (u8*)(reason), __LINE__, (u8*)__FILE__), \
		__builtin_trap(), \
		0 \
	) \
)

void* _mem;
size_t _cur;
size_t _len;
EXPORT void initialise_mem(void* mem, size_t len){
	_mem = mem;
	_cur = 0;
	_len = len;
}
EXPORT void keep_alive(void* p){printf("Keeping %p alive\n",p);}
EXPORT void clear_mem(){_cur = 0;}
void* alloc(size_t len){
	if (_cur + len >= _len){return NULL;}
	_cur += len;
	return (void*)(&((uint8_t*)_mem)[_cur-len]);
}

EXPORT utf8 test_codepoint_to_utf8(codepoint g){
	size_t len = codepoint_to_utf8(g, (utf8){0});
	utf8 rv = {alloc(len), len};
	assert(codepoint_to_utf8(g, rv) == len, "the length of the output should be determenistic");
	return rv;
}


EXPORT utf16 test_codepoint_to_utf16(codepoint g){
	size_t len = codepoint_to_utf16(g, (utf16){0});
	utf16 rv = {alloc(len), len};
	assert(codepoint_to_utf16(g, rv) == len, "the length of the output should be determenistic");
	return rv;
}

EXPORT codepoint test_codepoint_from_utf8(utf8 text){
	codepoint_return g = codepoint_from_utf8(text);
	assert(g.consumed == text.len, "did not read all the text (could be that there was more than a single codepoint)");
	return g.codepoint;
}

EXPORT codepoint test_codepoint_from_utf16(utf16 text){
	codepoint_return g = codepoint_from_utf16(text);
	assert(g.consumed == text.len, "did not read all the text (could be that there was more than a single codepoint)");
	return g.codepoint;
}
