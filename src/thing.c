#include "thing.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

thing* things = NULL;

thing* get_things(){
	if (things != NULL){return things;}
	things = malloc(sizeof(thing) * 5);
	things->name.chars = "te1st";
	things->name.len = strlen(things->name.chars);
	things->children = &things[1];
	things->child_len = 4;
	things->properties = NULL;
	things->property_len = 0;
	for (int i = 1; i < 5; i++){
		int size = snprintf(NULL, 0, "child %d", i);
		things[i].name.chars = malloc(size+1);
		snprintf(things[i].name.chars, size+1, "child %d", i);
		things[i].name.len = size;
		things[i].children = NULL;
		things[i].child_len = 0;
		things[i].properties = NULL;
		things[i].property_len = 0;
	}
	return things;
}

