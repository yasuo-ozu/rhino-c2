#include "rh_common.h"

void *rh_malloc(size_t size) {
	return malloc(size);
}

void rh_free(void *addr) {
	free(addr);
}

void *rh_malloc_string(char *s) {
	void *p = rh_malloc(strlen(s) + 1);
	strcpy(p, s);
	return p;
}
