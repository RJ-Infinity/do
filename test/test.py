import sys
import ctypes
from typing import Any

def struct(name: str, fields: list[tuple[str, Any]])->type:
	return type(name, (ctypes.Structure,), {"_fields_": fields})

c_link = ctypes.CDLL(".\\build\\test\\win\\obj\\test\\test.c.dll")

typedefs = type("", (), {})
typedefs.u32 = ctypes.c_uint32
typedefs.u16 = ctypes.c_uint16
typedefs.u8 = ctypes.c_uint8
typedefs.codepoint = typedefs.u32
typedefs.utf32 = struct("utf32", [
	("chars", ctypes.POINTER(typedefs.u32)),
	("len", ctypes.c_size_t),
])
typedefs.utf16 = struct("utf16", [
	("chars", ctypes.POINTER(typedefs.u16)),
	("len", ctypes.c_size_t),
])
typedefs.utf8 = struct("utf8", [
	("chars", ctypes.POINTER(typedefs.u8)),
	("len", ctypes.c_size_t),
])
typedefs.assert_fn = ctypes.CFUNCTYPE(
	None,
	# args
	ctypes.POINTER(typedefs.u8),
	ctypes.POINTER(typedefs.u8),
	ctypes.c_int,
	ctypes.POINTER(typedefs.u8)
)

c_link.set_assert.argtypes = typedefs.assert_fn,
c_link.set_assert.restype = None
c_link.initialise_mem.argtypes = ctypes.c_void_p, ctypes.c_size_t,
c_link.initialise_mem.restype = None
c_link.keep_alive.argtypes = ctypes.c_void_p,
c_link.keep_alive.restype = None
c_link.clear_mem.argtypes = ()
c_link.clear_mem.restype = None
c_link.test_codepoint_to_utf8.argtypes = typedefs.codepoint,
c_link.test_codepoint_to_utf8.restype = typedefs.utf8
c_link.test_codepoint_to_utf16.argtypes = typedefs.codepoint,
c_link.test_codepoint_to_utf16.restype = typedefs.utf16
c_link.test_codepoint_from_utf8.argtypes = typedefs.utf8,
c_link.test_codepoint_from_utf8.restype = typedefs.codepoint
c_link.test_codepoint_from_utf16.argtypes = typedefs.utf16,
c_link.test_codepoint_from_utf16.restype = typedefs.codepoint


def call_c_fn(*args):
	pass

def get_next_unicode():
	# yield 0x30000
	# yield 0x30001
	# yield 0x30001000
	c = 0xfffc
	while c < 0x30000:
		# skip surrogates (as python dosent like them)
		# they really shouldnt ever be encoded but most software supports them anyway
		if c >= 0xD800 and c <= 0xDFFF: c = 0xDFFF
		else: yield c
		c+=1

def get_str(cstr, clen = None, format = "utf-8", bytesize = 1):
	cstr = ctypes.cast(cstr, ctypes.POINTER(typedefs.u8))
	if clen == None:
		clen = 0
		while cstr[clen] != 0: clen+=1
	else:
		clen *= bytesize
	b = bytes(cstr[0:clen])
	if format != "none":
		b = b.decode(format)
	return b

def c_assert(condition, reason, line: int, file):
	print(f"assertion `{get_str(condition)}` failed in c code {get_str(file)}:{line} becuase {get_str(reason)}")
def main():
	assert_fn_ptr = typedefs.assert_fn(c_assert)
	c_link.set_assert(assert_fn_ptr)
	p = (ctypes.c_int8*256)()
	c_link.initialise_mem(p, 256)

	u_gen = get_next_unicode()
	for u in u_gen:
		c_link.clear_mem()
		utf8 = c_link.test_codepoint_to_utf8(u)
		string = get_str(utf8.chars, utf8.len, "none")
		correct = chr(u).encode("utf-8")
		if (string != correct):
			print(f"{hex(u)}\nstring {string}\ncorrect {correct}\n{string == correct}\n")
			break
	print("converting codepoint to utf8 correct")

	u_gen = get_next_unicode()
	for u in u_gen:
		c_link.clear_mem()
		utf16 = c_link.test_codepoint_to_utf16(u)
		string = get_str(utf16.chars, utf16.len, "none", 2)
		correct = chr(u).encode("utf-16le")
		if (string != correct):
			print(f"{hex(u)}\nstring {string}\ncorrect {correct}\n{string == correct}\n")
			break
	print("converting codepoint to utf16 correct")

	u_gen = get_next_unicode()
	for u in u_gen:
		c_link.clear_mem()
		utf = chr(u).encode("utf-8")
		g = c_link.test_codepoint_from_utf8(typedefs.utf8(ctypes.cast(ctypes.c_char_p(utf), ctypes.POINTER(typedefs.u8)), len(utf)))
		if (g != u):
			print(f"string {repr(utf)}\ncorrect {hex(u)}\nreturned {hex(g)}\n{g == u}\n")
			break
	print("converting utf8 to codepoint correct")

	u_gen = get_next_unicode()
	for u in u_gen:
		c_link.clear_mem()
		utf = chr(u).encode("utf-16le")
		g = c_link.test_codepoint_from_utf16(typedefs.utf16(ctypes.cast(ctypes.c_char_p(utf), ctypes.POINTER(typedefs.u16)), len(utf)//2))
		if (g != u):
			print(f"string {repr(utf)}\ncorrect {hex(u)}\nreturned {hex(g)}\n{g == u}\n")
			break
	print("converting utf16 to codepoint correct")

	c_link.keep_alive(p)
	c_link.keep_alive(assert_fn_ptr)



if __name__ == "__main__": main()