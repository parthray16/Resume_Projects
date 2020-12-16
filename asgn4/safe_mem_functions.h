#ifndef SAFEMEMFUNCTIONS
#define SAFEMEMFUNCTIONS

#include <stdlib.h>

void *safe_malloc(size_t);
void *safe_realloc(void *, size_t);
void *safe_calloc(int, size_t);
#endif