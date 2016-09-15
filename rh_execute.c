#include "rh_common.h"

rh_token *token_next(rh_context *ctx) {
	return ctx->token = ctx->token->next;
}

int token_cmp(rh_token *token, char *ident) {
	return !strcmp(token->text, ident);
}

int token_cmp_skip(rh_context *ctx, char *ident) {
	int ret = token_cmp(ctx->token, ident);
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
	OP_PREFIX = 2, OP_POSTFIX = 4, OP_BINARY = 1, OP_CONDITIONAL
} rh_operator_type;

int get_priority(rh_token *token, rh_operator_type type) {
	static struct {
		char *symbol; int priority; int type;
	} priority_table[] = {
		{"++", 1, OP_POSTFIX}, {"--", 1, OP_POSTFIX}, {"++", 2, OP_PREFIX}, {"--", 2, OP_PREFIX}, {"+", 2, OP_PREFIX},
		{"-", 2, OP_PREFIX}, {"~", 2, OP_PREFIX}, {"!", 2, OP_PREFIX}, {"*", 4, OP_BINARY}, {"/", 4, OP_BINARY},
		{"%", 4, OP_BINARY}, {"+", 5, OP_BINARY}, {"-", 5, OP_BINARY}, {"<<", 6, OP_BINARY}, {">>", 6, OP_BINARY},
		{"<", 7, OP_BINARY}, {"<=", 7, OP_BINARY}, {">", 7, OP_BINARY}, {">=", 7, OP_BINARY}, {"==", 8, OP_BINARY},
		{"!=", 8, OP_BINARY}, {"&", 9, OP_BINARY}, {"^", 10, OP_BINARY}, {"|", 11, OP_BINARY}, {"&&", 12, OP_BINARY},
		{"&&", 13, OP_BINARY}, {"&&", 14, OP_BINARY}, {"=", 15, OP_BINARY}, {"+=", 15, OP_BINARY}, {"-=", 15, OP_BINARY}, 
		{"*=", 15, OP_BINARY}, {"/=", 15, OP_BINARY}, {"%=", 15, OP_BINARY}, {"<<=", 15, OP_BINARY}, {">>=", 15, OP_BINARY}, 
		{"&=", 15, OP_BINARY}, {"^=", 15, OP_BINARY}, {"|=", 15, OP_BINARY}, {",", 16, OP_BINARY}, {0, 0, OP_BINARY}
	};
	int i;
	for (i = 0; priority_table[i].symbol; i++) {
		if (priority_table[i].type & type && token_cmp(token, priority_table[i].symbol)) {
			return priority_table[i].priority;
		}
	}
	return -1;
}

typedef enum {
	EM_DISABLED = 0, EM_ENABLED = 1
} rh_execute_mode;

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

rh_variable *rh_execute_expression_internal_term(rh_context *ctx, rh_execute_mode execMode) {
	rh_variable *ret = NULL;
	if (ctx->token->type == TYP_LITERAL) ret = ctx->token->variable;
	else if (ctx->token->type == TYP_IDENT) ret = search_declarator(ctx, ctx->token->text);
	else if (token_cmp(ctx->token, "(")) ret = expression_with_paren(ctx, execMode);
	if (ret == NULL) {
		E_ERROR(ctx, "Invalid endterm '%s'", ctx->token->text);
	}
	return ret;
}

int is_equal_operator(char *text) {
	static char *eqopTable[] = {
		"&&=", "||=", "<<=", ">>=", "+=", "-=", "*=", "/=", "|=", "&=", "^=", NULL
	};
	for (int i = 0; eqopTable[i] != NULL; i++) {
		if (strcmp(eqopTable[i], text) == 0) return 1;
	}
	return 0;
}

rh_variable *rh_execute_calculation_pre(rh_context *ctx, rh_variable *var, rh_token *token) {
	return NULL;
}

rh_variable *rh_execute_calculation_binary(rh_context *ctx, rh_variable *var1, rh_variable *var2, rh_token *token) {
	if (var1 == NULL || var2 == NULL) return NULL;
	if ((var1->type->kind == RHTYP_NUMERIC && var2->type->kind == RHTYP_FLOATING) ||
			(var1->type->kind == RHTYP_NUMERIC && var2->type->kind == RHTYP_POINTER)) {
		rh_variable *tmp = var1; var1 = var2; var2 = tmp;
	}
	if (var1->type->kind == RHTYP_NUMERIC && var2->type->kind == RHTYP_NUMERIC) {
		rh_type *type = rh_init_type();
		type->kind = RHTYP_NUMERIC;
		type->size = MAX(var1->type->size, var2->type->size);
		type->sign = MIN(var1->type->sign, var2->type->sign);
		rh_variable *ret = rh_init_variable(type);
		long long int1 = 0, int2 = 0, int3 = 0;
		memcpy(&int1, var1->memory, var1->type->size);
		memcpy(&int2, var2->memory, var2->type->size);
		if      (token_cmp(token, "+"))		int3 = int1 +  int2;
		else if (token_cmp(token, "-"))		int3 = int1 -  int2;
		else if (token_cmp(token, "*"))		int3 = int1 *  int2;
		else if (token_cmp(token, "/"))		int3 = int1 /  int2;
		else if (token_cmp(token, "%"))		int3 = int1 %  int2;
		else if (token_cmp(token, "<<"))	int3 = int1 << int2;
		else if (token_cmp(token, ">>"))	int3 = int1 >> int2;
		else if (token_cmp(token, "<"))		int3 = int1 <  int2;
		else if (token_cmp(token, "<="))	int3 = int1 <= int2;
		else if (token_cmp(token, ">"))		int3 = int1 >  int2;
		else if (token_cmp(token, ">="))	int3 = int1 >= int2;
		else if (token_cmp(token, "=="))	int3 = int1 == int2;
		else if (token_cmp(token, "!="))	int3 = int1 != int2;
		else if (token_cmp(token, "&"))		int3 = int1 &  int2;
		else if (token_cmp(token, "^"))		int3 = int1 ^  int2;
		else if (token_cmp(token, "|"))		int3 = int1 |  int2;
		else if (token_cmp(token, "&&"))	int3 = int1 && int2;
		else if (token_cmp(token, "||"))	int3 = int1 || int2;
		else if (token_cmp(token, ","))		int3 = int2;
		else {
			E_ERROR(ctx, "Operator '%s' error", token->text);
			return NULL;
		}
		memcpy(ret->memory, &int3, type->size);
		return ret;
	} else if (var1->type->kind == RHTYP_FLOATING) {
		// TODO
	} else if (var1->type->kind == RHTYP_POINTER && var2->type->kind == RHTYP_NUMERIC) {
		// TODO
	}
	E_ERROR(ctx, "Operator '%s' type error", token->text);
	return NULL;
}

rh_variable *rh_execute_calculation_post(rh_context *ctx, rh_variable *var, rh_token *token) {
	return NULL;
}

rh_variable *rh_execute_calculation_equal(rh_context *ctx, rh_variable *var1, rh_variable *var2, rh_token *token) {
	return NULL;
}

rh_variable *rh_execute_expression_internal(rh_context *ctx, int priority, rh_execute_mode execMode, int isVector) {
	if (priority == 0) return rh_execute_expression_internal_term(ctx, execMode);
	else {
		rh_variable *ret = NULL, *var;
		rh_token *token = ctx->token, *token1;
		if (get_priority(token, OP_PREFIX)) {
			token_next(ctx);
			ret = rh_execute_expression_internal(ctx, priority, execMode, isVector);
			if (ret != NULL && execMode == EM_ENABLED) ret = rh_execute_calculation_pre(ctx, ret, token);
		} else {
			rh_execute_expression_internal(ctx, priority - 1, EM_DISABLED, isVector);
			if (get_priority(ctx->token, OP_BINARY) == priority) {
				if (is_equal_operator(ctx->token->text)) {
					rh_token *buf[20];
					int count = 0;
					do {
						buf[count++] = ctx->token; token_next(ctx);
						rh_execute_expression_internal(ctx, priority - 1, EM_DISABLED, isVector);
					} while (get_priority(ctx->token, OP_BINARY) == priority &&
							is_equal_operator(ctx->token->text));
					if (execMode == EM_ENABLED) {
						token1 = ctx->token;
						ctx->token = buf[--count]->next;
						ret = rh_execute_expression_internal(ctx, priority - 1, EM_ENABLED, isVector);
						while (count > 0) {
							ctx->token = buf[count - 1]->next;
							var = rh_execute_expression_internal(ctx, priority - 1, EM_ENABLED, isVector);
							ret = rh_execute_calculation_equal(ctx, var, ret, buf[count--]);
						}
						ctx->token = token;
						var = rh_execute_expression_internal(ctx, priority - 1, EM_ENABLED, isVector);
						ret = rh_execute_calculation_equal(ctx, var, ret, buf[0]);
						ctx->token = token1;
					}
				} else {
					do {
						// TODO: %%,||を使用時にオペランド1の結果によってオペランド2を評価するかしないか判断
						token1 = ctx->token;
						if (execMode == EM_ENABLED) {
							ctx->token = token;
							ret = rh_execute_expression_internal(ctx, priority - 1, execMode, isVector);
						}
						token_next(ctx);
						var = rh_execute_expression_internal(ctx, priority - 1, execMode, isVector);
						if (execMode == EM_ENABLED) ret = rh_execute_calculation_binary(ctx, ret, var, token1);
					} while (get_priority(ctx->token, OP_BINARY) == priority);
				}
			} else {
				if (execMode == EM_ENABLED) {
					ctx->token = token;
					ret = rh_execute_expression_internal(ctx, priority - 1, execMode, isVector);
				}
				while (get_priority(ctx->token, OP_POSTFIX) == priority) {
					if (isVector && token_cmp(ctx->token, ",")) break;
					token1 = ctx->token;
					if (token_cmp_skip(ctx, "++") || token_cmp_skip(ctx, "--"))
						if (execMode == EM_ENABLED) ret = rh_execute_calculation_post(ctx, ret, token1);
				}
			}
		}
		return ret;
	}
}

rh_variable *rh_execute_expression(rh_context *ctx, rh_execute_mode execMode, int isVector) {
	return rh_execute_expression_internal(ctx, 16, execMode, isVector);
}

typedef enum {
	SR_NORMAL = 0, SR_RETURN, SR_CONTINUE, SR_BREAK
} rh_statement_result;

rh_statement_result rh_execute_statement(rh_context *ctx, rh_execute_mode execMode) {
	int needsSemicolon = 1;
	rh_statement_result res = SR_NORMAL;
	if (token_cmp_skip(ctx, "if")) {

	}
}

int rh_execute(rh_context *ctx) {
	return 0;
}

