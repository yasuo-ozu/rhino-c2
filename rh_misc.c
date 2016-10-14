#include "rh_common.h"

void *rh_malloc(rh_context *ctx, size_t size) {
	void *p = malloc(size);
	if (p == NULL) {
		E_FATAL(ctx, "memory allocating error");
	}
	return p;
}

void rh_free(rh_context *ctx, void *addr) {
	UNUSED(ctx);
	free(addr);
}

char *rh_malloc_string(rh_context *ctx, char *s) {
	char *ret = rh_malloc(ctx, strlen(s) + 1);
	strcpy(ret, s);
	return ret;
}
