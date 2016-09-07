#include "rh_common.h"

void *rh_malloc(size_t size) {
	return malloc(size);
}

void rh_free(void *addr) {
	free(addr);
}
