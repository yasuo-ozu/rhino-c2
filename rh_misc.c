#include "rh_common.h"

void *rh_malloc(rh_context *ctx, size_t size) {
	void *p = malloc(size);
	if (p == NULL) {
		E_FATAL(ctx, "memory allocating error");
	}
	return p;
}

void rh_free(void *addr) {
	free(addr);
}

void *rh_malloc_string(char *s) {
	void *p = rh_malloc(strlen(s) + 1);
	strcpy(p, s);
	return p;
}
