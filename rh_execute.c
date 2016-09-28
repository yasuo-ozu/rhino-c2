#include "rh_common.h"

 rh_token *token_next(rh_context *ctx) {
	if (ctx->token == NULL)
		return ctx->token = rh_next_token(ctx);
	else {
		if (ctx->token->next == NULL) ctx->token->next = rh_next_token(ctx);
	 	return ctx->token = ctx->token->next;
	}
 }

#define token_cmp(token, /* (char *) */ ident) \
	((token) != NULL && (token)->type != TYP_LITERAL && !strcmp((token)->text, (ident)))

#define token_fetch(ctx) \
	((ctx)->token == NULL ? ((ctx)->token = token_next(ctx)) : (ctx)->token)

int token_cmp_skip(rh_context *ctx, char *ident) {
	int ret = token_cmp(token_fetch(ctx), ident);
	if (ret) token_next(ctx);
	return ret;
}

int token_cmp_error_skip(rh_context *ctx, char *ident) {
	int ret = token_cmp_skip(ctx, ident);
	if (!ret) {
		E_ERROR(ctx, "requires '%s'", ident);
	}
	return ret;
}

typedef enum {
	EM_DISABLED = 0, EM_ENABLED = 1
} rh_execute_mode;

typedef enum {
	OP_PREFIX = 2, OP_POSTFIX = 4, OP_BINARY = 1, OP_CONDITIONAL
} rh_operator_type;

typedef struct {
	enum {
		SR_NORMAL = 0, SR_RETURN, SR_CONTINUE, SR_BREAK
	} type;
	rh_variable *var;
} rh_statement_result;

int get_priority(rh_token *token, rh_operator_type type) {
	static struct {
		char *symbol; int priority; int type;
	} priority_table[] = {
		{"[", 1, OP_POSTFIX}, {"++", 1, OP_POSTFIX}, {"--", 1, OP_POSTFIX}, 
		{"++", 2, OP_PREFIX}, {"--", 2, OP_PREFIX}, {"+", 2, OP_PREFIX}, {"*", 2, OP_PREFIX}, {"&", 2, OP_PREFIX},
		{"-", 2, OP_PREFIX}, {"~", 2, OP_PREFIX}, {"!", 2, OP_PREFIX}, {"*", 4, OP_BINARY}, {"/", 4, OP_BINARY},
		{"%", 4, OP_BINARY}, {"+", 5, OP_BINARY}, {"-", 5, OP_BINARY}, {"<<", 6, OP_BINARY}, {">>", 6, OP_BINARY},
		{"<", 7, OP_BINARY}, {"<=", 7, OP_BINARY}, {">", 7, OP_BINARY}, {">=", 7, OP_BINARY}, {"==", 8, OP_BINARY},
		{"!=", 8, OP_BINARY}, {"&", 9, OP_BINARY}, {"^", 10, OP_BINARY}, {"|", 11, OP_BINARY}, {"&&", 12, OP_BINARY},
		{"||", 13, OP_BINARY}, {"?", 14, OP_BINARY}, {"=", 15, OP_BINARY}, {"+=", 15, OP_BINARY}, {"-=", 15, OP_BINARY}, 
		{"*=", 15, OP_BINARY}, {"/=", 15, OP_BINARY}, {"%=", 15, OP_BINARY}, {"<<=", 15, OP_BINARY}, {">>=", 15, OP_BINARY}, 
		{"&=", 15, OP_BINARY}, {"^=", 15, OP_BINARY}, {"|=", 15, OP_BINARY}, {",", 16, OP_BINARY}, {0, 0, OP_BINARY}
	};
	int i;
	if (token == NULL || token->type != TYP_SYMBOL) return -1;
	for (i = 0; priority_table[i].symbol; i++) {
		if (priority_table[i].type & type && token_cmp(token, priority_table[i].symbol)) {
			return priority_table[i].priority;
		}
	}
	return -1;
}

rh_variable *search_declarator(rh_context *ctx, char *text) {
	rh_variable *var = ctx->variable;
	while (var != NULL) {
		if (strcmp(var->token->text, text) == 0) break;
		var = var->next;
	}
	return var;
}

rh_variable *rh_execute_expression(rh_context *ctx, rh_execute_mode execMode, int isVector);
rh_variable *expression_with_paren(rh_context *ctx, rh_execute_mode execMode) {
	token_cmp_error_skip(ctx, "(");
	rh_variable *ret = rh_execute_expression(ctx, execMode, 0);
	token_cmp_error_skip(ctx, ")");
	return ret;
}

rh_statement_result rh_execute_statement(rh_context *ctx, rh_execute_mode execMode);
rh_variable *rh_execute_expression_functioncall(rh_context *ctx, rh_variable *func, rh_execute_mode execMode) {
	token_cmp_error_skip(ctx, ")");	// TODO: 本来なら引数を処理
	rh_token *token0 = token_fetch(ctx);
	ctx->token = func->func_body;
	rh_statement_result res = rh_execute_statement(ctx, execMode);
	ctx->token = token0;
	if (execMode == EM_DISABLED) return NULL;
	rh_variable *var = res.var;
	if (res.type != SR_RETURN) {
		if (strcmp(func->token->text, "main") != 0) {
			E_ERROR(ctx, "func return error");
		}
		rh_type *type = rh_init_type();
		type->kind = RHTYP_NUMERIC;
		type->size = 4; type->sign = 1;
		var = rh_init_variable(type);
		(*(int *) var->memory) = 0;
	}
	return rh_convert_variable(ctx, var, func->type, 0);
}

rh_variable *rh_execute_expression_internal_term(rh_context *ctx, rh_execute_mode execMode) {
	rh_variable *ret = NULL;
	rh_token *token = token_fetch(ctx);
	if (token->type == TYP_LITERAL) {
		ret = token->variable;
		token = token_next(ctx);
	} else if (token->type == TYP_IDENT) {
		ret = search_declarator(ctx, token->text);
		token = token_next(ctx);
		if (ret->args_count != -1 && token_cmp_skip(ctx, "(")) {	// Function call
			ret = rh_execute_expression_functioncall(ctx, ret, execMode);
		}
	} else if (token_cmp(token, "(")) ret = expression_with_paren(ctx, execMode);
	else {
		E_ERROR(ctx, "Invalid endterm '%s'", ctx->token->text);
		token_next(ctx);
	}
	return ret;
}

int is_equal_operator(char *text) {
	static char *eqopTable[] = {
		"&&=", "||=", "<<=", ">>=", "+=", "-=", "*=", "/=", "|=", "&=", "^=", "=", NULL
	};
	for (int i = 0; eqopTable[i] != NULL; i++) {
		if (strcmp(eqopTable[i], text) == 0) return 1;
	}
	return 0;
}

int variable_get_long_double(rh_variable *var, long double *dblval) {
	if (var->type->kind == RHTYP_NUMERIC) {
		long long intval = 0;
		memcpy(&intval, var->memory, var->type->size);
		*dblval = (long double) intval;
	} else if (var->type->kind == RHTYP_FLOATING) {
		if (var->type->size == 4) *dblval = (long double) *(float *) var->memory;
		else if (var->type->size == 8) *dblval = (long double) *(double *) var->memory;
		else if (var->type->size == 16) *dblval = *(long double *) var->memory;
		else return 0;
	} else return 0;
	return 1;
}

rh_variable *rh_execute_calculation_pre(rh_context *ctx, rh_variable *var, rh_token *token) {
	if (var == NULL) return NULL;
	rh_variable *ret;
	rh_type *type = var->type;
	if (var->type->kind == RHTYP_NUMERIC) {
		long long int1 = 0, int2 = 0;
		memcpy(&int1, var->memory, type->size);
		if      (token_cmp(token, "+"))	 int2 =   int1;
		else if (token_cmp(token, "-"))  int2 =  -int1;
		else if (token_cmp(token, "!"))  int2 =  !int1;
		else if (token_cmp(token, "~"))	 int2 =  ~int1;
		else if (token_cmp(token, "++")) int2 = ++int1;
		else if (token_cmp(token, "--")) int2 = --int1;
		else {
			E_ERROR(ctx, "Operator '%s' error", token->text);
			return NULL;
		}
		ret = rh_init_variable(type);
		memcpy(ret->memory, &int2, type->size);
		if (token_cmp(token, "++") || token_cmp(token, "--"))
			memcpy(var->memory, &int1, type->size);
		return ret;
	} else if (var->type->kind == RHTYP_FLOATING) {
		long double dbl1, dbl2;
		variable_get_long_double(var, &dbl1);
		if      (token_cmp(token, "+"))	dbl2 =   dbl1;
		else if (token_cmp(token, "-")) dbl2 =  -dbl1;
		else if (token_cmp(token, "!")) dbl2 =  !dbl1;
		else {
			E_ERROR(ctx, "Operator '%s' error", token->text);
			return NULL;
		}
		ret = rh_init_variable(type);
		if      (type->size ==  4) (*(float *)			ret->memory) = (float) dbl2;
		else if (type->size ==  8) (*(double *)			ret->memory) = (double) dbl2;
		else if (type->size == 16) (*(long double *)	ret->memory) = dbl2;
		return ret;
	} else if (var->type->kind == RHTYP_POINTER) {
		if (token_cmp(token, "*")) {
			int addr;
			memcpy(&addr, var->memory, sizeof(int));
			ret = rh_init_variable(NULL);
			ret->type = type->child;
			ret->memory = ctx->memory + addr;
			ret->address = addr;
			return ret;
		} else if (token_cmp(token, "&")) {
			if (var->address < 0) {
				E_ERROR(ctx, "pointer error");
				return NULL;
			}
			rh_type *type2 = rh_init_type();
			type2->kind = RHTYP_POINTER;
			type2->size = sizeof(int);
			type->child = type;
			ret = rh_init_variable(type2);
			(*(int *)ret->memory) = var->address;
			return ret;
		}
	}
	E_ERROR(ctx, "pre-op error");
	return NULL;
}

rh_variable *rh_execute_calculation_binary_internal(rh_context *ctx, rh_variable *var1, rh_variable *var2, char *op) {
	if ((var1->type->kind == RHTYP_NUMERIC && var2->type->kind == RHTYP_FLOATING) ||
			(var1->type->kind == RHTYP_NUMERIC && var2->type->kind == RHTYP_POINTER)) {
		rh_variable *tmp = var1; var1 = var2; var2 = tmp;
	}
	rh_type *type = rh_init_type();
	type->size = MAX(var1->type->size, var2->type->size);
	rh_variable *ret = rh_init_variable(type);
	if (var1->type->kind == RHTYP_NUMERIC && var2->type->kind == RHTYP_NUMERIC) {
		type->kind = RHTYP_NUMERIC;
		type->sign = MIN(var1->type->sign, var2->type->sign);
		long long int1 = 0, int2 = 0, int3 = 0;
		memcpy(&int1, var1->memory, var1->type->size);
		memcpy(&int2, var2->memory, var2->type->size);
		if      (strcmp(op,  "+") == 0)	int3 = int1 +  int2;
		else if (strcmp(op,  "-") == 0)	int3 = int1 -  int2;
		else if (strcmp(op,  "*") == 0)	int3 = int1 *  int2;
		else if (strcmp(op,  "/") == 0)	int3 = int1 /  int2;
		else if (strcmp(op,  "%") == 0)	int3 = int1 %  int2;
		else if (strcmp(op, "<<") == 0)	int3 = int1 << int2;
		else if (strcmp(op, ">>") == 0)	int3 = int1 >> int2;
		else if (strcmp(op,  "<") == 0)	int3 = int1 <  int2;
		else if (strcmp(op, "<=") == 0)	int3 = int1 <= int2;
		else if (strcmp(op,  ">") == 0)	int3 = int1 >  int2;
		else if (strcmp(op, ">=") == 0)	int3 = int1 >= int2;
		else if (strcmp(op, "==") == 0)	int3 = int1 == int2;
		else if (strcmp(op, "!=") == 0)	int3 = int1 != int2;
		else if (strcmp(op,  "&") == 0)	int3 = int1 &  int2;
		else if (strcmp(op,  "^") == 0)	int3 = int1 ^  int2;
		else if (strcmp(op,  "|") == 0)	int3 = int1 |  int2;
		else if (strcmp(op, "&&") == 0)	int3 = int1 && int2;
		else if (strcmp(op, "||") == 0)	int3 = int1 || int2;
		else if (strcmp(op,  ",") == 0)	int3 = int2;
		else {
			E_ERROR(ctx, "Operator '%s' error", op);
			rh_free_variable(ret);
			rh_free_type(type);
			return NULL;
		}
		memcpy(ret->memory, &int3, type->size);
		return ret;
	} else if (var1->type->kind == RHTYP_FLOATING) {
		type->kind = RHTYP_FLOATING;
		long double dbl1 = 0, dbl2 = 0, dbl3 = 0;
		if (!variable_get_long_double(var1, &dbl1) ||
			!variable_get_long_double(var2, &dbl2)) {
			E_ERROR(ctx, "Invalid operand type");
			rh_free_variable(ret);
			rh_free_type(type);
			return NULL;
		}
		if      (strcmp(op,  "+") == 0)	dbl3 = dbl1 +  dbl2;
		else if (strcmp(op,  "-") == 0)	dbl3 = dbl1 -  dbl2;
		else if (strcmp(op,  "*") == 0)	dbl3 = dbl1 *  dbl2;
		else if (strcmp(op,  "/") == 0)	dbl3 = dbl1 /  dbl2;
		else if (strcmp(op,  "<") == 0)	dbl3 = dbl1 <  dbl2;
		else if (strcmp(op, "<=") == 0)	dbl3 = dbl1 <= dbl2;
		else if (strcmp(op,  ">") == 0)	dbl3 = dbl1 >  dbl2;
		else if (strcmp(op, ">=") == 0)	dbl3 = dbl1 >= dbl2;
		else if (strcmp(op, "==") == 0)	dbl3 = dbl1 == dbl2;
		else if (strcmp(op, "!=") == 0)	dbl3 = dbl1 != dbl2;
		else if (strcmp(op, "&&") == 0)	dbl3 = dbl1 && dbl2;
		else if (strcmp(op, "||") == 0)	dbl3 = dbl1 || dbl2;
		else if (strcmp(op,  ",") == 0)	dbl3 = dbl2;
		else {
			E_ERROR(ctx, "Operator '%s' error", op);
			rh_free_variable(ret);
			rh_free_type(type);
			return NULL;
		}
		if (type->size == 4) (*(float *)ret->memory) = (float) dbl3;
		if (type->size == 8) (*(double *)ret->memory) = (double) dbl3;
		if (type->size == 16) (*(long double *)ret->memory) = dbl3;
		return ret;
	} else if (var1->type->kind == RHTYP_POINTER && var2->type->kind == RHTYP_NUMERIC) {

	}
	E_ERROR(ctx, "Operator '%s' type error", op);
	rh_free_variable(ret);
	rh_free_type(type);
	return NULL;
}

rh_variable *rh_execute_calculation_binary(rh_context *ctx, rh_variable *var1, rh_variable *var2, rh_token *token) {
	if (var1 == NULL || var2 == NULL) return NULL;
	return rh_execute_calculation_binary_internal(ctx, var1, var2, token->text);
}

rh_variable *rh_execute_calculation_post(rh_context *ctx, rh_variable *var, rh_token *token) {
	if (var == NULL) return NULL;
	rh_type *type = rh_dup_type(var->type);
	rh_variable *ret = rh_init_variable(type);
	if (var->type->kind == RHTYP_NUMERIC) {
		long long int1 = 0, int2 = 0;
		memcpy(&int1, var->memory, type->size);
		if (token_cmp(token, "++")) int2 = int1++;
		else if (token_cmp(token, "--")) int2 = int1--;
		else {
			E_ERROR(ctx, "Operator '%s' error", token->text);
			rh_free_variable(ret);
			rh_free_type(type);
			return NULL;
		}
		memcpy(ret->memory, &int2, type->size);
		memcpy(var->memory, &int1, type->size);
		return ret;
	}
	E_ERROR(ctx, "post-op error");
	return NULL;
}

rh_variable *rh_execute_calculation_equal(rh_context *ctx, rh_variable *var1, rh_variable *var2, rh_token *token) {
	if (var1 == NULL || var2 == NULL) return NULL;
	char buf[RH_TOKEN_MAXLEN];
	rh_variable *ret = NULL;
	if (token_cmp_skip(ctx, "=")) {
		ret = var2;
	} else {
		strcpy(buf, token->text);
		buf[strlen(buf) - 1] = '\0';
		ret = rh_execute_calculation_binary_internal(ctx, var1, var2, buf);
	}
	if (ret != NULL) rh_assign_variable(ctx, var1, ret);
	return ret;
}

rh_variable *rh_execute_expression_internal(rh_context *ctx, int priority, rh_execute_mode execMode, int isVector) {
	if (priority == 0) return rh_execute_expression_internal_term(ctx, execMode);
	else {
		rh_variable *ret = NULL, *var;
		rh_token *token = token_fetch(ctx), *token0 = token, *token1;
		if (get_priority(token, OP_PREFIX) == priority) {
			token_next(ctx);
			ret = rh_execute_expression_internal(ctx, priority, execMode, isVector);
			if (ret != NULL && execMode == EM_ENABLED) ret = rh_execute_calculation_pre(ctx, ret, token_fetch(ctx));
			token = token_fetch(ctx);
		} else {
			rh_execute_expression_internal(ctx, priority - 1, EM_DISABLED, isVector);
			token = token_fetch(ctx);
			if (!(isVector && token_cmp(token, ",")) && get_priority(token, OP_BINARY) == priority) {
				if (is_equal_operator(token->text)) {
					rh_token *buf[20];
					int count = 0;
					do {
						buf[count++] = token; token = token_next(ctx);
						rh_execute_expression_internal(ctx, priority - 1, EM_DISABLED, isVector);
						token = token_fetch(ctx);
					} while (get_priority(token, OP_BINARY) == priority &&
							is_equal_operator(token->text));
					if (execMode == EM_ENABLED) {
						token1 = token;
						ctx->token = buf[--count]->next;
						ret = rh_execute_expression_internal(ctx, priority - 1, EM_ENABLED, isVector);
						token = token_fetch(ctx);
						while (count > 0) {
							ctx->token = buf[count - 1]->next;
							var = rh_execute_expression_internal(ctx, priority - 1, EM_ENABLED, isVector);
							ret = rh_execute_calculation_equal(ctx, var, ret, buf[count--]);
						}
						ctx->token = token0;
						var = rh_execute_expression_internal(ctx, priority - 1, EM_ENABLED, isVector);
						ret = rh_execute_calculation_equal(ctx, var, ret, buf[0]);
						ctx->token = token1;
					}
				} else {
					if (execMode == EM_ENABLED) {
						ctx->token = token0;
						ret = rh_execute_expression_internal(ctx, priority - 1, execMode, isVector);
						token = token_fetch(ctx);
					}
					while (!(isVector && token_cmp(token, ",")) && get_priority(token, OP_BINARY) == priority) {
						// TODO: %%,||を使用時にオペランド1の結果によってオペランド2を評価するかしないか判断
						token1 = token;
						token = token_next(ctx);
						var = rh_execute_expression_internal(ctx, priority - 1, execMode, isVector);
						if (execMode == EM_ENABLED) ret = rh_execute_calculation_binary(ctx, ret, var, token1);
						token = token_fetch(ctx);
					}
				}
			} else {
				if (execMode == EM_ENABLED) {
					ctx->token = token0;
					ret = rh_execute_expression_internal(ctx, priority - 1, execMode, isVector);
					token = token_fetch(ctx);
				}
				while (get_priority(token, OP_POSTFIX) == priority) {
					if (isVector && token_cmp(token, ",")) break;
					if (token_cmp_skip(ctx, "[")) {
						rh_variable *indexVal = rh_execute_expression(ctx, execMode, 1);
						token_cmp_error_skip(ctx, "]");
						if (execMode == EM_ENABLED) {
							int i, j;
							if (ret == NULL || !rh_variable_to_int(ctx, indexVal, &i)
									|| !(ret->type->kind == RHTYP_POINTER || ret->type->kind == RHTYP_ARRAY)) {
								E_ERROR(ctx, "array iterator error");
							} else {
								memcpy(&j, ret->memory, sizeof(int));
								var = rh_init_variable(NULL);
								var->type = ret->type->child;
								var->memory = ctx->memory + j + var->type->size * i;
								var->address = j + var->type->size * i;
								ret = var;
							}
						}
					} else {
						token1 = token;
						if (token_cmp_skip(ctx, "++") || token_cmp_skip(ctx, "--"))
							if (execMode == EM_ENABLED) ret = rh_execute_calculation_post(ctx, ret, token1);
					}
					token = token_fetch(ctx);
				}
			}
		}
		return ret;
	}
}

rh_variable *rh_execute_expression(rh_context *ctx, rh_execute_mode execMode, int isVector) {
	rh_variable *res = NULL;
	rh_token *token = token_fetch(ctx), *token0 = token;
	if (token->child[1] != NULL && execMode == EM_DISABLED) {
		ctx->token = token->child[1];
		return NULL;
	}
	res = rh_execute_expression_internal(ctx, 16, execMode, isVector);
	token = token_fetch(ctx);
	token0->child[1] = token;
	return res;
}

rh_type *read_type_speifier(rh_context *ctx, rh_execute_mode execMode) {
	rh_type *ret = NULL;
	int signedCount = 0, unsignedCount = 0, longCount = 0, size = 0, count = 0;
	for (;;) {
		if (token_cmp_skip(ctx, "unsigned")) unsignedCount++, size = 4;
		else if (token_cmp_skip(ctx, "signed")) signedCount++, size = 4;
		else if (token_cmp_skip(ctx, "long")) longCount++, size = 4;
		else if (token_cmp_skip(ctx, "char")) size = 1, count++;
		else if (token_cmp_skip(ctx, "short")) size = 2, count++;
		else if (token_cmp_skip(ctx, "int")) size = 4, count++;
		else break;
	}
	if (size > 0) {
		if (count > 1 || signedCount + unsignedCount > 1 || longCount > 2 || (longCount && size != 4)) {
			E_ERROR(ctx, "Type error");
			return NULL;
		}
		ret = rh_init_type();
		if (longCount == 2) size = 8;
		ret->size = size;
		ret->sign = signedCount - unsignedCount;
		ret->kind = RHTYP_NUMERIC;
	}
	return ret;
}

rh_type *read_type_declarator(rh_context *ctx, rh_type *parent, rh_token **pToken, int identMode, rh_execute_mode execMode) {
	rh_type *ret = NULL;
	rh_variable *var = NULL;
	if (token_cmp_skip(ctx, "(")) {
		ret = read_type_declarator(ctx, parent, pToken, identMode, execMode);
		token_cmp_error_skip(ctx, ")");
	} else if (token_cmp_skip(ctx, "*")) {
		ret = read_type_declarator(ctx, parent, pToken, identMode, execMode);
		rh_type *type = rh_init_type();
		type->child = ret;
		type->kind = RHTYP_POINTER;
		type->size = 4;
		ret = type;
	} else {
		ret = parent;
		rh_token *token = token_fetch(ctx);
		if (token->type == TYP_IDENT) {
			if (identMode == -1) {
				E_ERROR(ctx, "Unexpected identifier");
			} else if (pToken != NULL) *pToken = token;
			token_next(ctx);
		}
	}
	while (token_cmp_skip(ctx, "[")) {
		if (!token_cmp(ctx->token, "]"))
			var = rh_execute_expression(ctx, execMode, 1);
		token_cmp_error_skip(ctx, "]");
		if (execMode == EM_DISABLED) return NULL;
		rh_type *type = rh_init_type();
		int i;
		if (var != NULL) {
			rh_variable_to_int(ctx, var, &i);
			rh_free_variable(var);
			type->length = i;
			type->size = ret->size * i;
		} else {
			type->length = -1;
		}
		type->kind = RHTYP_ARRAY;
		type->child = ret;
		ret = type;
	}
	return ret;
}

int execute_brace_initializer(rh_context *ctx, rh_type *type, rh_execute_mode execMode) {
	if (type->kind == RHTYP_POINTER || type->kind == RHTYP_ARRAY) {
		int i = 0, count = -1, nest = 0, size = rh_get_typesize(type->child);
		rh_variable *baseVar = rh_init_variable(NULL), *var2;
		baseVar->type = type->child;
		if (type->kind == RHTYP_ARRAY) count = type->length;
		do {
			if (token_cmp_skip(ctx, "{")) {
				if (!execute_brace_initializer(ctx, type->child, execMode)) nest++;
			} else if (token_cmp(ctx->token, "}")) {
				if (--nest < 0) break;
				token_next(ctx);
			} else {
				var2 = rh_execute_expression(ctx, execMode, 1);
				if ((count == -1 || i < count) && execMode == EM_ENABLED) {
					baseVar->memory = ctx->memory + ctx->hp;
					rh_assign_variable(ctx, baseVar, var2);
					ctx->hp += size; i++;
				}
			}
		} while (token_cmp_skip(ctx, ","));
		token_cmp_error_skip(ctx, "}");
		while (count != -1 && i < count) {
			memset(ctx->memory + ctx->hp, 0, size);
			ctx->hp += size; i++;
		}
		if (count == -1 && type->kind == RHTYP_ARRAY) type->length = i;
		return 1;
	}
	return 0;
}

rh_statement_result rh_execute_statement(rh_context *ctx, rh_execute_mode execMode);
rh_variable *rh_execute_statement_variable(rh_context *ctx, rh_execute_mode execMode, rh_type *type, rh_token *idToken) {
	rh_variable *var, *var2;
	if (execMode == EM_ENABLED) {
		var = rh_init_variable(NULL);	// TODO: 関数内ではスタックに確保する
		var->type = type;
		var->token = idToken;
		var->is_left = 1;
		if (type->kind == RHTYP_ARRAY) {
			var->address = -1;
			var->memory = rh_malloc(sizeof(int));	///< 配列が確保される先の仮想アドレスが入る
			(*(int *)var->memory) = ctx->hp;
		} else {
			var->address = ctx->hp;
			var->memory = ctx->memory + ctx->hp;
			ctx->hp += type->size;
		}
	}
	if (token_cmp_skip(ctx, "=")) {
		if (token_cmp_skip(ctx, "{")) {
			if (execMode == EM_ENABLED) {
				if (type->kind == RHTYP_POINTER) (*(int *)var->memory) = ctx->hp;
				else if (type->kind != RHTYP_ARRAY) ctx->hp -= type->size;
			}
			if (!execute_brace_initializer(ctx, type, execMode)) {
				E_ERROR(ctx, "variable initialize error");
			}
		} else {
			var2 = rh_execute_expression(ctx, execMode, 1);
			if (execMode == EM_ENABLED){
				rh_assign_variable(ctx, var, var2);
			}
		}
	} else {
		if (execMode == EM_ENABLED) {
			memset(var->memory, 0, type->size);
		}
	}
	if (type->kind == RHTYP_ARRAY && type->length == -1) {
		E_ERROR(ctx, "length error");
		type->length = 1;
	}
	type->size = rh_get_typesize(type);
	return var;
}

rh_variable *rh_execute_statement_function(rh_context *ctx, rh_execute_mode execMode, rh_type *type, rh_token *idToken0) {
	rh_token *token = token_fetch(ctx), *idToken;
	rh_variable *var = rh_init_variable(NULL), *var1, *varTop = NULL;
	rh_type *type0, *type1;
	int nullTokenCount = 0;
	var->args_count = 0;
	var->args = NULL;
	var->token = idToken0;
	var->type = type;
	while (!token_cmp(token, ")")) {
		type0 = read_type_speifier(ctx, execMode);
		if (type0 == NULL) {	// int
			type0 = rh_init_type();
			type0->kind = RHTYP_NUMERIC;
			type0->size = 4; type->sign = 1;
		}
		type1 = read_type_declarator(ctx, type0, &idToken, 0, execMode);	// idToken maybe null
		if (type1 == NULL) nullTokenCount++;
		var1 = rh_init_variable(NULL);
		var1->type = type1;
		var1->token = idToken;
		if (var->args == NULL) var->args = varTop = var1;
		else {
			varTop->next = var1;
			varTop = var1;
		}
		var->args_count++;
		if (!token_cmp_skip(ctx, ",")) break;
		token = token_fetch(ctx);
	}
	token_cmp_error_skip(ctx, ")");
	token = token_fetch(ctx);
	if (nullTokenCount && token_cmp(token, "{")) {
		E_ERROR(ctx, "func decl err");
		return NULL;
	}
	if (token_cmp(token, "{")) {
		var->func_body = token;		// compound statement (function body)
		rh_execute_statement(ctx, EM_DISABLED);
	} else {
		token_cmp_error_skip(ctx, ";");
	}
	return var;
}

rh_statement_result rh_execute_statement_type(rh_context *ctx, rh_execute_mode execMode, rh_type *type0) {
	rh_variable *var;
	rh_type *type;
	rh_statement_result res = {SR_NORMAL, NULL};
	rh_token *idToken;
	int isFunction = 0;
	do {
		rh_token *idToken;
		type = read_type_declarator(ctx, type0, &idToken, 1, execMode);
		if (type != NULL && idToken != NULL) {
			for (var = ctx->variable; var != ctx->variable_top; var = var->next) {
				if (strcmp(var->token->text, idToken->text) == 0 && var->func_body != NULL) {
					E_ERROR(ctx, "The name '%s' is already in use.", idToken->text);
					execMode = EM_DISABLED;
				}
			}
			if (token_cmp_skip(ctx, "(")) {
				var = rh_execute_statement_function(ctx, execMode, type, idToken);
				isFunction = 1;
			} else {
				var = rh_execute_statement_variable(ctx, execMode, type, idToken);
			}
			if (execMode == EM_ENABLED && var != NULL) {
				var->next = ctx->variable;
				ctx->variable = var;
			}
			if (isFunction) break;
		} else {
			E_ERROR(ctx, "type decl err.");
		}
	} while (token_fetch(ctx) != NULL && token_cmp_skip(ctx, ","));
	if (!isFunction) token_cmp_error_skip(ctx, ";");
	return res;
}

rh_statement_result rh_execute_statement(rh_context *ctx, rh_execute_mode execMode) {
	int needsSemicolon = 1, i;
	rh_statement_result res = {SR_NORMAL, NULL};
	rh_token *token = token_fetch(ctx), *token0 = token, *token1;
	rh_variable *var;
	if (token->child[0] != NULL && execMode == EM_DISABLED) {
		ctx->token = token->child[0];
		return res;
	}
	if (token_cmp_skip(ctx, "if")) {
		var = expression_with_paren(ctx, execMode);
		rh_variable_to_int(ctx, var, &i);
		res = rh_execute_statement(ctx, execMode && i);
		if (token_cmp_skip(ctx, "else"))
			res = rh_execute_statement(ctx, execMode && !i && res.type == SR_NORMAL);
		needsSemicolon = 0;
	} else if (token_cmp_skip(ctx, "while")) {
		token1 = token_fetch(ctx);
		for (;;) {
			var = expression_with_paren(ctx, execMode);
			rh_variable_to_int(ctx, var, &i);
			res = rh_execute_statement(ctx, execMode && i);
			if (!execMode || !i || res.type == SR_BREAK) {
				res.type = SR_NORMAL; break;
			} else if (res.type == SR_RETURN) break;
			ctx->token = token1;
		}
		needsSemicolon = 0;
	} else if (token_cmp_skip(ctx, "do")) {
		token1 = token_fetch(ctx);
		for (;;) {
			res = rh_execute_statement(ctx, execMode);
			token_cmp_error_skip(ctx, "while");
			var = expression_with_paren(ctx, execMode);
			if (execMode == EM_DISABLED || res.type == SR_RETURN) break;
			if (res.type == SR_BREAK) {
				res.type = SR_NORMAL; break;
			}
			rh_variable_to_int(ctx, var, &i);
			if (!i) break;
			ctx->token = token1;
		}
	} else if (token_cmp_skip(ctx, "for")) {
		token_cmp_error_skip(ctx, "(");
		rh_execute_expression(ctx, execMode, 0);
		token_cmp_error_skip(ctx, ";");
		rh_token *token2 = token_fetch(ctx);
		for (;;) {
			var = rh_execute_expression(ctx, execMode, 0);
			rh_variable_to_int(ctx, var, &i);
			token_cmp_error_skip(ctx, ";");
			token1 = token_fetch(ctx);
			rh_execute_expression(ctx, EM_DISABLED, 0);
			token_cmp_error_skip(ctx, ")");
			res = rh_execute_statement(ctx, execMode && i);
			if (execMode == EM_DISABLED || !i || res.type == SR_BREAK) {
				res.type = SR_NORMAL; break;
			} else if (res.type == SR_RETURN) break;
			ctx->token = token1;
			rh_execute_expression(ctx, execMode, 0);
			ctx->token = token2;
		}
		needsSemicolon = 0;
	} else if (token_cmp_skip(ctx, "{")) {
		rh_statement_result res2;
		rh_variable *varTop = ctx->variable_top;
		ctx->variable_top = ctx->variable;
		token = token_fetch(ctx);
		while (token != NULL && !token_cmp(token, "}")) {
			res2 = rh_execute_statement(ctx, execMode && res.type == SR_NORMAL);
			token = token_fetch(ctx);
			if (res2.type != SR_NORMAL) res = res2;
		}
		token_cmp_error_skip(ctx, "}");
		while (ctx->variable != ctx->variable_top) {
			rh_variable *tmpVar = ctx->variable;
			ctx->variable = tmpVar->next;
			rh_free_variable(tmpVar);
		}
		ctx->variable_top = varTop;
		needsSemicolon = 0;
	} else if (token_cmp(token, ";"));
	else if (token_cmp_skip(ctx, "break"))   { if (execMode == EM_ENABLED) res.type = SR_BREAK;    }
	else if (token_cmp_skip(ctx, "continue")){ if (execMode == EM_ENABLED) res.type = SR_CONTINUE; }
	else if (token_cmp_skip(ctx, "return"))  { 
		if (execMode == EM_ENABLED) res.type = SR_RETURN;
		if (token_cmp(token_fetch(ctx), ";")) {
			if (execMode == EM_ENABLED) {
				res.var = rh_init_variable(NULL);
				res.var->type = rh_init_type();
				res.var->type->kind = RHTYP_VOID;
				res.var->type->size = 0;
			}
		} else {
			res.var = rh_execute_expression(ctx, execMode, 0);
		}
	} else {
		rh_type *type = read_type_speifier(ctx, execMode);
		if (type != NULL) {
			res = rh_execute_statement_type(ctx, execMode, type);
			needsSemicolon = 0;
		} else {
			rh_variable *var = rh_execute_expression(ctx, execMode, 0);
			if (execMode == EM_ENABLED && (ctx->flag & RHFLAG_INTERACTIVE)) {
				printf("= ");
				rh_dump_variable(ctx, var);
			}
		}
	}
	if (needsSemicolon) token_cmp_error_skip(ctx, ";");
	token0->child[0] = token_fetch(ctx);
	return res;
}

int rh_execute(rh_context *ctx) {
	if (ctx->flag & RHFLAG_INTERACTIVE) {
		printf(">> ");
	}
	while (token_fetch(ctx) != NULL) {
		rh_execute_statement(ctx, EM_ENABLED);
		if ((ctx->flag & RHFLAG_INTERACTIVE) && ctx->error.count) {
			rh_dump_error(ctx);
		}	
		if (ctx->flag & RHFLAG_INTERACTIVE) {
			printf(">> ");
		}
	}
	return 0;
}

