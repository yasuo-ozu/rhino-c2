#include "rh_common.h"

rh_variable *rh_init_variable(rh_type *type) {
	rh_variable *var = rh_malloc(sizeof(rh_variable));
	var->token = NULL;
	var->type = type;
	var->memory = NULL;
	var->address = -2;
	var->next = NULL;
	var->is_dynamic = 0;
	var->is_left = 0;
	var->args = NULL;
	var->args_count = -1;
	var->func_body = NULL;
	if (type != NULL && type->size > 0) {
		var->memory = rh_malloc(rh_get_typesize(type));
		var->address = -1;
	}
	return var;
}
/*
rh_variable *rh_init_variable_local(rh_context *ctx, rh_type *type, int depth, int isStatic) {
	rh_variable *ret = rh_init_variable(NULL);
	int size = type->size;
	ret->type = type;
	if (depth == 0 || isStatic) {
		ret->memory = ctx->memory + ctx->hp;
		ret->address = ctx->hp;
		ctx->hp += size;
		ctx->hp = (ctx->hp + 7) & 0xFFFFFFF8;
	} else {
		ctx->sp -= size;
		ctx->sp &= 0xFFFFFFF8;
		ret->memory = ctx->memory + ctx->sp;
		ret->address = ctx->sp;
	}
	return ret;	
}
*/

void rh_free_variable(rh_variable *var) {
	if (var->address == -1 && var->memory != NULL) {
		rh_free(var->memory);
	}
	rh_free(var);
}

void rh_dump_variable_internal(rh_context *ctx, unsigned char *mem, rh_type *type) {
	if (type->kind == RHTYP_NUMERIC) {
		long long intval = 0;
		memcpy(&intval, mem, rh_get_typesize(type));
		if (type->size == 1) printf("'%c' ", (char) intval);
		printf("%d", (int) intval);
	} else if (type->kind == RHTYP_POINTER) {
		long long intval = 0;
		memcpy(&intval, mem, rh_get_typesize(type));
		printf("<%d> ", (int) intval);
		if (type->kind == RHTYP_POINTER && type->child->kind == RHTYP_NUMERIC && type->child->size == 1) {
			printf("\"%s\" ", ctx->memory + intval);
		} else {
			rh_dump_variable_internal(ctx, mem, type->child);
		}
	} else if (type->kind == RHTYP_ARRAY) {
		long long intval = 0;
		memcpy(&intval, mem, rh_get_typesize(type));
		printf("<%d> {", (int) intval);
		for (int i = 0; i < type->length; i++) {
			if (i > 0) printf(", ");
			rh_dump_variable_internal(ctx, ctx->memory + (int) intval + i * type->child->size, type->child);
		}
		printf("} ");
	} else if (type->kind == RHTYP_FLOATING) {
		double dblval = 0;
		if (type->size == 16) dblval = (double) *(long double *) mem;
		if (type->size == 8) dblval = (double) *(double *) mem;
		if (type->size == 4) dblval = (double) *(float *) mem;
		printf("%lf ", dblval);
	}
}

void rh_dump_variable(rh_context *ctx, rh_variable *var) {
	if (var == NULL) {
		printf("NULL\n");
		return;
	}
	printf("(");
	rh_dump_type(var->type);
	printf(") ");
	rh_dump_variable_internal(ctx, var->memory, var->type);
	printf("\n");
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
			if (!force && type->size < var->type->size) {
				E_WARNING(ctx, "numeric size");
			}
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
			if (!force && type->size < var->type->size) {
				E_WARNING(ctx, "floating size");
			}
			if (type->size ==  4) (*(float *)var->memory) = (float) dblval;
			if (type->size ==  8) (*(double *)var->memory) = (double) dblval;
			if (type->size == 16) (*(long double *)var->memory) = (long double) dblval;
			return ret;
		}
	} else if (var->type->kind == RHTYP_POINTER) {
		if (type->kind == RHTYP_POINTER) {
			if (var->type->child->kind != type->child->kind &&  !force && 
					var->type->child->kind != RHTYP_VOID && type->child->kind != RHTYP_VOID) {
				E_ERROR(ctx, "pointer convert error");
			}
			memcpy(ret->memory, var->memory, type->size);
			return ret;
		} else if (type->kind == RHTYP_NUMERIC) {
			if (!force) {
				E_ERROR(ctx, "pointer convert error");
			}
			long long longval = 0;
			memcpy(&longval, var->memory, var->type->size);
			memcpy(ret->memory, &longval, type->size);
			return ret;
		}
	}
	if (var->type->kind != RHTYP_VOID || type->kind != RHTYP_VOID) {
		E_ERROR(ctx, "type convert error");
	}
	if (ret->memory != NULL) memset(ret->memory, 0, type->size);
	return ret;
}

int rh_variable_to_int(rh_context *ctx, rh_variable *var, int *intval) {
	if (var == NULL) return 0;
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
	rh_convert_variable(ctx, src, dest->type, 0);
	memcpy(dest->memory, src->memory, dest->type->size);
}

