#ifndef _THING_H
#define _THING_H
#include "unicode.h"
#include <stdbool.h>


typedef struct {
	enum {
		TP_CHECKBOX,
	} type;
	union{
		struct{
			bool checked;
		} checkbox;
	};
} thing_property;

struct thing;
typedef struct thing{
	utf8 name;

	struct thing* children;
	size_t child_len;

	thing_property* properties;
	size_t property_len;
} thing;


thing* get_things();
#endif