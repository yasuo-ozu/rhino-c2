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
	rh_dump_type(var->type);
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


