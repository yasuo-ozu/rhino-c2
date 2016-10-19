#include "rh_common.h"

static rh_parse *rh_init_parse(rh_context *ctx) {
	UNUSED(ctx);
	rh_parse *ps = rh_malloc(ctx, sizeof(rh_parse));
	ps->type = PSTYP_NULL;
	ps->next = NULL;
	ps->var = NULL;
	ps->token = NULL;
	for (int i = 0; i < 4; i++) ps->child[i] = NULL;
	return ps;
}

static void rh_dump_parse_internal_expression(rh_context *ctx, rh_parse *ps) {
	if (ps->type == PSTYP_VAR) {
		rh_dump_variable(ctx, ps->var);
	} else {
		printf("( ");
		if (ps->type == PSTYP_PREOP) printf("<PRE: '%s'> ", ps->token->text);
		rh_dump_parse_internal_expression(ctx, ps->child[0]);
		if (ps->type == PSTYP_POSTOP) printf("<PST: '%s'> ", ps->token->text);
		else if (ps->type == PSTYP_BINARYOP) {
			printf("<BIN: '%s'> ", ps->token->text);
			rh_dump_parse_internal_expression(ctx, ps->child[1]);
		}
		printf(") ");
	}
}

static void rh_dump_parse_internal(rh_context *ctx, rh_parse *ps, int level) {
	char indent[32];
	int i;
	for (i = 0; i < level; i++) indent[i] = '\t';
	indent[i] = '\0';
	//
	if      (ps->type == PSTYP_NULL)		printf("%sNULL\n", indent);
	else if (ps->type == PSTYP_FUNC_DECL)	printf("%sFUNC_DECL\n", indent);
	else if (ps->type == PSTYP_BREAK)		printf("%sBREAK\n", indent);
	else if (ps->type == PSTYP_RETURN)		printf("%sRETURN\n", indent);
	else if (ps->type == PSTYP_CONTINUE)	printf("%sCONTINUE\n", indent);
	else if (ps->type == PSTYP_VOIDSTAT)	printf("%sVOIDSTAT\n", indent);
	else if (ps->type == PSTYP_IF) {
		printf("%sIF\n", indent);
		rh_dump_parse_internal(ctx, ps->child[0], level + 1);
		rh_dump_parse_internal(ctx, ps->child[1], level + 1);
		if (ps->child[2] != NULL) rh_dump_parse_internal(ctx, ps->child[2], level + 1);
	} else if (ps->type == PSTYP_WHILE){
		printf("%sWHILE\n", indent);
		rh_dump_parse_internal(ctx, ps->child[0], level + 1);
		rh_dump_parse_internal(ctx, ps->child[1], level + 1);
	} else if (ps->type == PSTYP_DOWHILE){
		printf("%sDOWHILE\n", indent);
		rh_dump_parse_internal(ctx, ps->child[0], level + 1);
		rh_dump_parse_internal(ctx, ps->child[1], level + 1);
	} else if (ps->type == PSTYP_FOR) {
		printf("%sFOR\n", indent);
		rh_dump_parse_internal(ctx, ps->child[0], level + 1);
		rh_dump_parse_internal(ctx, ps->child[1], level + 1);
		rh_dump_parse_internal(ctx, ps->child[2], level + 1);
		rh_dump_parse_internal(ctx, ps->child[3], level + 1);
	} else if (ps->type == PSTYP_COMPOUND){
		printf("%s{\n", indent);
		for (rh_parse *ps2 = ps->child[0]; ps2 != NULL; ps2 = ps2->next) {
			rh_dump_parse_internal(ctx, ps2, level + 1);
		}
		printf("%s}\n", indent);
	} else if (ps->type == PSTYP_VARDECL)		printf("%sVARDECL\n", indent);
	else if (ps->type == PSTYP_EXPRESSION) {
		printf("%sEXPRESSION: ", indent);
		rh_dump_parse_internal_expression(ctx, ps->child[0]);
		printf("\n");
	} else	printf("%sOTHER\n", indent);
}

void rh_dump_parse(rh_context *ctx, rh_parse *ps) {
	rh_dump_parse_internal(ctx, ps, 0);
}

static rh_token *token_next(rh_context *ctx) {
	if (ctx->token_next == NULL) {
		ctx->token_next = rh_next_token(ctx);
	}
	if (ctx->token != NULL) {
		ctx->token->next = ctx->token_disposed;
		ctx->token_disposed = ctx->token;
	}
	ctx->token 		= ctx->token_next;
	ctx->token_next	= ctx->token_next->next; 
	return ctx->token;
}

static void token_push(rh_context *ctx, rh_token *tkn) {
	rh_token *token, *tokenBefore = NULL;
	for (token = ctx->token_disposed; token != NULL; token = token->next) {
		if (token == tkn) {
			if (tokenBefore == NULL) ctx->token_disposed = token->next;
			else tokenBefore->next = token->next;
			break;
		}
		tokenBefore = token;
	}
	tkn->next = ctx->token_next;
	ctx->token_next = tkn;
	ctx->token = NULL;
}

#define token_cmp(token, /* (char *) */ ident) \
	((token) != NULL && (token)->type != TYP_LITERAL && (token)->type != TYP_NULL && !strcmp((token)->text, (ident)))

#define token_fetch(ctx) ((ctx)->token == NULL ? token_next(ctx) : (ctx)->token)

#define token_eof(ctx)	(token_fetch(ctx)->type == TYP_NULL)

// int token_cmp_skip(rh_context *ctx, char *ident) {
// 	rh_token *token0 = token_fetch(ctx);
// 	int ret = token_cmp(token0, ident);
// 	if (ret) token_next(ctx);
// 	return ret;
// }

int token_cmp_error(rh_context *ctx, char *ident) {
	int ret = token_cmp(token_fetch(ctx), ident);
	if (!ret) {
		E_ERROR(ctx, "requires '%s'", ident);
	}
	return ret;
}

int token_skip_cmp(rh_context *ctx, char *ident) {
	rh_token *token = token_next(ctx);
	int ret = token_cmp(token, ident);
	if (!ret) token_push(ctx, token);
	return ret;
}

int token_skip_cmp_error(rh_context *ctx, char *ident) {
	int ret = token_skip_cmp(ctx, ident);
	if (!ret) {
		E_ERROR(ctx, "requires '%s'", ident);
	}
	return ret;
}

// int token_cmp_error_skip(rh_context *ctx, char *ident) {
// 	int ret = token_cmp_skip(ctx, ident);
// 	if (!ret) {
// 		E_ERROR(ctx, "requires '%s'", ident);
// 	}
// 	return ret;
// }

typedef enum {
	OP_PREFIX = 2, OP_POSTFIX = 4, OP_BINARY = 1, OP_CONDITIONAL
} rh_operator_type;

static int get_operator_priority(rh_token *token, rh_operator_type type, int isVector) {
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
			if (priority_table[i].priority == 16 && isVector) return -1;
			else return priority_table[i].priority;
		}
	}
	return -1;
}

rh_parse *rh_parse_expression(rh_context *ctx, const int isVector);
rh_parse *rh_parse_expression_internal(rh_context *ctx, const int isVector, const int priority) {
	rh_parse *ret = NULL;
	rh_token *token = token_next(ctx);
	if (priority == 0) {		// 項の処理
		if (token->type == TYP_LITERAL) {
			ret = rh_init_parse(ctx);
			ret->type = PSTYP_VAR;
			ret->var = token->variable;
		} else if (token->type == TYP_IDENT) {	// 変数の処理
			E_ERROR(ctx, "Not implemented.");
		} else if (token_cmp(token, "(")) {
			ret = rh_parse_expression(ctx, isVector);
			token_skip_cmp_error(ctx, ")");
		} else {
			E_ERROR(ctx, "Invalid endterm '%s'", token->text);
		}
	} else if (get_operator_priority(token, OP_PREFIX, isVector) == priority) {	// 前置演算子の処理
		ret = rh_init_parse(ctx);
		ret->type = PSTYP_PREOP;
		ret->token = token;
		ret->child[0] = rh_parse_expression_internal(ctx, isVector, priority);
		return ret;
	} else {
		token_push(ctx, token);
		ret = rh_parse_expression_internal(ctx, isVector, priority - 1);
		token = token_next(ctx);
		if (get_operator_priority(token, OP_BINARY, isVector) == priority) {	// 二項演算子の処理
			if (priority == 15) {		// 右結合
				rh_parse *pss[32];
				rh_token *tks[32];
				int i = 0;
				pss[i] = ret;
				do {
					tks[i++] = token;
					pss[i] = rh_parse_expression_internal(ctx, isVector, priority - 1);
					token = token_next(ctx);
				} while (get_operator_priority(token, OP_BINARY, isVector) == priority);
				token_push(ctx, token);
				ret = pss[i];
				while (i-- > 0) {
					rh_parse *ps = rh_init_parse(ctx);
					ps->type = PSTYP_BINARYOP;
					ps->child[0] = pss[i];
					ps->child[1] = ret;
					ps->token = tks[i];
					ret = ps;
				}
			} else {
				rh_parse *ps = rh_init_parse(ctx);
				ps->type = PSTYP_BINARYOP;
				ps->token = token;
				ps->child[0] = ret;
				ps->child[1] = rh_parse_expression_internal(ctx, isVector, priority);
				ret = ps;
			}
		} else {
			while (get_operator_priority(token, OP_POSTFIX, isVector) == priority) {	// 後置演算子の処理
				rh_parse *ps = rh_init_parse(ctx);
				ps->type = PSTYP_POSTOP;
				ps->token = token;
				ps->child[0] = ret;
				ret = ps;
				token = token_next(ctx);
			}
			token_push(ctx, token);
		}
	}
	return ret;
}

rh_parse *rh_parse_expression(rh_context *ctx, const int isVector) {
	rh_parse *ps = rh_init_parse(ctx);
	ps->type = PSTYP_EXPRESSION;
	ps->child[0] = rh_parse_expression_internal(ctx, isVector, 16);
	return ps;
}

rh_parse *rh_parse_statement(rh_context *ctx) {
	int semi = 1;
	rh_parse *ps = rh_init_parse(ctx);
	rh_token *token0 = token_next(ctx);	// １つめのトークンを取得
	if (token0->type == TYP_NULL) return NULL;
	if (token_cmp(token0, "if")) {
		ps->type = PSTYP_IF;
		token_skip_cmp_error(ctx, "(");
		ps->child[0] = rh_parse_expression(ctx, FALSE);
		token_skip_cmp_error(ctx, ")");
		ps->child[1] = rh_parse_statement(ctx);
		ps->child[2] = token_skip_cmp(ctx, "else") ? rh_parse_statement(ctx) : NULL;
		semi = 0;
	} else if (token_cmp(token0, "while")) {
		ps->type = PSTYP_WHILE;
		token_skip_cmp_error(ctx, "(");
		ps->child[0] = rh_parse_expression(ctx, FALSE);
		token_skip_cmp_error(ctx, ")");
		ps->child[1] = rh_parse_statement(ctx);
		semi = 0;
	} else if (token_cmp(token0, "do")) {
		ps->type = PSTYP_DOWHILE;
		ps->child[0] = rh_parse_statement(ctx);
		token_skip_cmp_error(ctx, "while");
		token_skip_cmp_error(ctx, "(");
		ps->child[1] = rh_parse_expression(ctx, FALSE);
		token_skip_cmp_error(ctx, ")");
	} else if (token_cmp(token0, "for")) {
		ps->type = PSTYP_FOR;
		token_skip_cmp_error(ctx, "(");
		ps->child[0] = rh_parse_expression(ctx, TRUE);
		token_skip_cmp_error(ctx, ";");
		ps->child[1] = rh_parse_expression(ctx, TRUE);
		token_skip_cmp_error(ctx, ";");
		ps->child[2] = rh_parse_expression(ctx, TRUE);
		token_skip_cmp_error(ctx, ")");
		ps->child[3] = rh_parse_statement(ctx);
		semi = 0;
	} else if (token_cmp(token0, "{")) {
		ps->type = PSTYP_COMPOUND;
		rh_parse *ps1 = NULL, *ps1_bottom = NULL;
		while (!token_eof(ctx) && !token_skip_cmp(ctx, "}")) {
			rh_parse *ps2 = rh_parse_statement(ctx);
			if (ps1 != NULL) ps1->next = ps2;
			else ps1_bottom = ps2;
			ps1 = ps2;
		}
		ps->child[0] = ps1_bottom;
		token_cmp_error(ctx, "}");
		semi = 0;
	} else if (token_cmp(token0, ";"))			ps->type = PSTYP_VOIDSTAT, semi = 0;
	else if (token_cmp(token0, "break"))		ps->type = PSTYP_BREAK;
	else if (token_cmp(token0, "return"))		ps->type = PSTYP_RETURN;
	else if (token_cmp(token0, "continue"))		ps->type = PSTYP_CONTINUE;
	else {
		// TODO: 宣言文
		token_push(ctx, token0);
		ps->type = PSTYP_EXPRESSION;
		ps->child[0] = rh_parse_expression(ctx, FALSE);
	}
	if (semi) token_skip_cmp_error(ctx, ";");
	return ps;
}

