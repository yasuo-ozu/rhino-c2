#include "rh_common.h"

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

void rh_dump_type(rh_type *type) {
	if (type->child != NULL) rh_dump_type(type->child);
	if (type->kind == RHTYP_POINTER) printf(" *");
	if (type->kind == RHTYP_ARRAY) printf("[%d]", type->length);
	if (type->kind == RHTYP_NUMERIC) {
		printf("%s %s", type->sign ? "signed" : "unsigned",
				type->size == 8 ? "long long" : type->size == 4 ? "int" : type->size == 2 ? "short": "char");
	}
	if (type->kind == RHTYP_FLOATING) {
		printf("%s", type->size == 16 ? "long double" : type->size == 8 ? "double" : "float");
	}
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

rh_variable *rh_convert_type(rh_context *ctx, rh_variable *var, int force) {
	UNUSED(ctx, var, force);
	return NULL;
}


