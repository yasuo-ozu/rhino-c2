#include "rh_common.h"

rh_variable *rh_init_variable(rh_type *type) {
	rh_variable *var = rh_malloc(sizeof(rh_variable));
	var->token = NULL;
	var->type = type;
	var->memory = NULL;
	var->next = NULL;
	var->is_dynamic = 0;
	var->is_left = 0;
	if (type != NULL) {
		var->memory = rh_malloc(rh_get_typesize(type));
	}
	return var;
}

void rh_free_variable(rh_variable *var) {
	if (var->is_dynamic && var->memory != NULL) {
		rh_free(var->memory);
	}
	rh_free(var);
}

void rh_dump_variable(rh_variable *var) {
	if (var == NULL) {
		printf("NULL\n");
		return;
	}
	printf("(");
	rh_dump_type(var->type);
	printf(") ");
	if (var->type->kind == RHTYP_NUMERIC || var->type->kind == RHTYP_POINTER) {
		long long intval = 0;
		memcpy(&intval, var->memory, rh_get_typesize(var->type));
		printf("%d\n", (int) intval);
	} else if (var->type->kind == RHTYP_FLOATING) {
		double dblval = 0;
		if (var->type->size == 16) dblval = (double) *(long double *) var->memory;
		if (var->type->size == 8) dblval = (double) *(double *) var->memory;
		if (var->type->size == 4) dblval = (double) *(float *) var->memory;
		printf("%lf\n", dblval);
	}
}

rh_variable *rh_search_variable(rh_context *ctx, char *ident) {
	rh_variable *decl = ctx->variable;
	while (decl) {
		if (strcmp(ident, decl->token->text) == 0) break;
		decl = decl->next;
	}
	return decl;
}

rh_variable *rh_convert_variable(rh_context *ctx, rh_variable *var, rh_type *type, int force) {
	rh_variable *ret = rh_init_variable(type);
	if (var->type->kind == RHTYP_NUMERIC) {
		long long intval = 0;
		memcpy(&intval, var->memory, var->type->size);
		if (type->kind == RHTYP_NUMERIC) {
			memcpy(ret->memory, &intval, type->size);
			return ret;
		} else if (type->kind == RHTYP_FLOATING) {
			if (type->size ==  4) (*(float *)ret->memory) 		= (float)intval;
			if (type->size ==  8) (*(double *)ret->memory) 		= (double)intval;
			if (type->size == 16) (*(long double *)ret->memory) = (long double)intval;
			return ret;
		}
	} else if (var->type->kind == RHTYP_FLOATING) {
		long double dblval;
		if (var->type->size ==  4) dblval = (long double)*(float *)var->memory;
		if (var->type->size ==  8) dblval = (long double)*(double *)var->memory;
		if (var->type->size == 16) dblval = (long double)*(long double *)var->memory;
		if (type->kind == RHTYP_NUMERIC) {
			long long intval = (long long) dblval;
			memcpy(ret->memory, &intval, type->size);
			return ret;
		} else if (type->kind == RHTYP_FLOATING) {
			if (type->size ==  4) (*(float *)var->memory) = (float) dblval;
			if (type->size ==  8) (*(double *)var->memory) = (double) dblval;
			if (type->size == 16) (*(long double *)var->memory) = (long double) dblval;
			return ret;
		}
	}
	E_ERROR(ctx, "type convert error");
	rh_free_variable(ret);
	return NULL;
}

int rh_variable_to_int(rh_context *ctx, rh_variable *var, int *intval) {
	rh_type *type = rh_init_type();
	type->size = 4; type->sign = 1; type->kind = RHTYP_NUMERIC;
	rh_variable *ret = rh_convert_variable(ctx, var, type, 0);
	if (ret == NULL) return 0;
	*intval = *(int *)ret->memory;
	rh_free_variable(ret);
	rh_free_type(type);
	return 1;
}

void rh_assign_variable(rh_context *ctx, rh_variable *dest, rh_variable *src) {
	rh_variable *var = rh_convert_variable(ctx, src, dest->type, 0);
	memcpy(dest->memory, src->memory, dest->type->size);
}


