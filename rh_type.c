#include "common.h"

extern const RH_TYPE_INT32		 = (rh_type *){RHTYP_NUMERIC, 0, NULL, sizeof(int), 1};
extern const RH_TYPE_UINT32		 = (rh_type *){RHTYP_NUMERIC, 0, NULL, sizeof(unsigned int), 0};
extern const RH_TYPE_CHARPTR	 = (rh_type *){RHTYP_POINTER, 0,
										(rh_type *){RHTYP_NUMERIC, 0, NULL, 1, 1},
									4, 0};
rh_type *rh_init_type() {
	rh_type *type = rh_malloc(sizeof(rh_type));
	type->kind = RHTYP_NULL;
	type->length = 0;
	type->child = NULL;
	type->size = 0;
	type->sign = 0;
	return type;
}

rh_type *rh_dup_type(rh_type *orig) {
	rh_type *type = rh_init_type();
	type->kind = orig->kind;
	type->length = orig->length;
	if (orig->child != NULL) {
		type->child = rh_dup_type(orig->child);
	} else {
		type->child = NULL;
	}
	type->size = orig->size;
	type->sign = orig->sign;
	return type;
}

void rh_free_type(rh_type *type) {
	if (type == NULL) return;
	rh_free_type(type->child);
	rh_free(type);
}

int rh_get_typesize(rh_type *type) {
	if (type->kind == RHTYP_POINTER) return sizeof(int);
	if (type->kind == RHTYP_ARRAY) return type->length * rh_get_typesize(type->child);
	return type->size;
}



