#include "rh_common.h"

rh_parse *rh_init_parse(rh_context *ctx) {
	UNUSED(ctx);
	rh_parse *ps = rh_malloc(ctx, sizeof(rh_parse));
	ps->type = PSTYP_NULL;
	ps->start = NULL;
	ps->stop = NULL;
	ps->next = NULL;
	for (int i = 0; i < 4; i++) ps->child[i] = NULL;
	return ps;
}

rh_token *token_next(rh_context *ctx) {
	if (ctx->token != ctx->token_top) {
		ctx->token = ctx->token->next;
	} else if (ctx->token == NULL || ctx->token->type != TYP_NULL) {
		rh_token *token = rh_next_token(ctx);
		if (ctx->token != NULL) ct->token->next = token;
		ctx->token = token;
		ctx->token_top = token;
		if (ctx->token_bottom == NULL) ctx->token_bottom = token;
	}
	return ctx->token;
}

#define token_cmp(token, /* (char *) */ ident) \
	((token) != NULL && (token)->type != TYP_LITERAL && !strcmp((token)->text, (ident)))

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
}

int token_skip_cmp(rh_context *ctx, char *ident) {
	rh_token *token0 = token_fetch(ctx), *token = token_next(ctx);
	int ret = token_cmp(token, ident);
	if (!ret) ctx->token = token0;
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

/*
rh_parse *rh_parse_global_item_type(rh_context *ctx, rh_type *type0) {
	rh_parse *ps;
	rh_type *type;
	int isFunc = 0;
	token_next(ctx);
	do {
		rh_token *idToken;
		type = read_type_declarator(ctx, type0, &idToken, 1);
		if (type != NULL && idToken != NULL) {
			// TODO: 名前の重複確認
			if (token_cmp_skip(ctx, "(")) {
				rh_parse_
			} else {

			}
			// TODO: 記号表への登録
}
*/

rh_parse *rh_parse_statement(rh_context *ctx) {
	int semi = 1;
	rh_parse *ps = rh_init_parse(ctx);
	rh_token *token0 = token_next(ctx);	// １つめのトークンを取得
	if (token_cmp(token0, "if")) {
		ps->type = PSTYP_IF;
		ps->child[0] = rh_parse_expression_with_paren(ctx);
		ps->child[1] = rh_parse_statement(ctx);
		ps->child[2] = token_skip_cmp(ctx, "else") ? rh_parse_statement(ctx) : NULL;
		semi = 0;
	} else if (token_cmp(token0, "while")) {
		ps->type = PSTYP_WHILE;
		ps->child[0] = rh_parse_expression_with_paren(ctx);
		ps->child[1] = rh_parse_statement(ctx);
		semi = 0;
	} else if (token_cmp(token0, "do")) {
		ps->type = PSTYP_DOWHILE;
		ps->child[0] = rh_parse_statement(ctx);
		token_skip_cmp_error(ctx, "while");
		ps->child[1] = rh_parse_expression_with_paren(ctx);
	} else if (token_cmp(token0, "for")) {
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
		rh_parse *ps1 = NULL, *ps1_bottom = NULL;
		while (!token_eof(ctx) && !token_skip_cmp(ctx, "}")) {
			rh_parse *ps2 = rh_parse_statement(ctx);
			if (ps1 != NULL) ps1->next = ps2;
			else ps1_bottom = ps2;
			ps1 = ps2;
		}
		ps->child[0] = ps1;
		token_skip_cmp_error(ctx, "}");
		semi = 0;
	} else if (token_cmp(token0, ";")) semi = 0;
	else if (token_cmp(token0, "break"))		ps->type = PSTYP_BREAK;
	else if (token_cmp(token0, "return"))		ps->type = PSTYP_RETURN;
	else if (token_cmp(token0, "continue"))		ps->type = PSTYP_CONTINUE;
	else {
		// TODO: 宣言文
		ps->type = PSTYP_EXPRESSION;
		ps->child[0] = rh_parse_expression(ctx, FALSE);
	}
	if (semi) token_skip_cmp_error(ctx, ";");
	return ps;
}

rh_parse *rh_parse_global_item(rh_context *ctx) {
	
}

rh_parse *rh_parse_global_decl(rh_context *ctx) {
	return rh_parse_global_item(ctx);
}
