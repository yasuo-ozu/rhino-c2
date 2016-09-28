#include "rh_common.h"

char *token_keyword_table[] = {
	"_Bool", "char", "short", "int", "long", "signed", "unsigned", "float", "double", "_Complex", "_Imaginary",
	"struct", "union", "enum", "volatile", "const", "restrict", "auto", "extern", "static", "register",
	"typedef", "void", "if", "else", "switch", "case", "default", "for", "do", "while", "goto", "continue", "break", "return",
	"inline", "sizeof", NULL
};

char *token_symbol_table[] = {
	/* 3 chars */
	"&&=", "||=", "<<=", ">>=",
	/* 2 chars */
	"&&", "||", "++", "--", "<<", ">>",  "+=", "-=", "*=", "/=", 
	"|=", "&=", "^=", "==", "!=", "<=", ">=", "->", 0
};

rh_token *rh_init_token(rh_context *ctx) {
	rh_token *token = rh_malloc(sizeof(rh_token));
	token->type = TYP_NULL;
	token->text = NULL;
	token->next = NULL;
	token->variable = NULL;
	token->child[0] = NULL;
	token->child[1] = NULL;
	token->child[2] = NULL;
	return token;
}

void rh_dump_token(rh_context *ctx, rh_token *token) {
	if (token->type == TYP_SYMBOL) {
		printf("SYMBOL: %s\n", token->text);
	} else if (token->type == TYP_IDENT) {
		printf("IDENT: %s\n", token->text);
	} else if (token->type == TYP_LITERAL) {
		printf("LITERAL: ");
		rh_dump_variable(ctx, token->variable);
	} else if (token->type == TYP_KEYWORD) {
		printf("KEYWORD: %s\n", token->text);
	}
}

void rh_next_token_ident(rh_context *ctx, char c, rh_token *token) {
	char s[RH_TOKEN_MAXLEN], *ch = s;
	token->type = TYP_IDENT;
	do {
		*ch++ = c; c = rh_getchar(ctx);
	} while (c == '_' || ('A' <= c && c <= 'Z') ||
			('a' <= c && c <= 'z') || ('0' <= c && c <= '9'));
	*ch++ = 0;
	token->text = rh_malloc_string(s);
	token->type = TYP_IDENT;
	for (int i = 0; token_keyword_table[i] != NULL; i++) {
		char *ch2 = token_keyword_table[i]; ch = s;
		do {
			if (*ch != *ch2) break;
		} while (*ch++ != '\0' && *ch2++ != '\0');
		if (*ch == '\0' && *ch2 == '\0') {
			token->type = TYP_KEYWORD;
			break;
		}
	}
	rh_ungetc(ctx, c);
}

void rh_next_token_numdbl(rh_context *ctx, char c, rh_token *token) {
	token->type = TYP_LITERAL;
	long long intval = 0; long double dblval;
	int isHex = 0, isDbl = 0, digit;
	if (c == '0') {
		c = rh_getchar(ctx);
		if (c == 'x' || c == 'X') {
			isHex = 1;
			for (;;) {
				c = rh_getchar(ctx);
				if ('0' <= c && c <= '9') digit = c - '0';
				else if ('A' <= c && c <= 'Z') digit = c - 'A' + 10;
				else if ('a' <= c && c <= 'z') digit = c - 'a' + 10;
				else break;
				intval = intval * 16 + digit;
			}
		} else {
			while ('0' <= c && c <= '7') {
				intval = intval * 8 + (c - '0');
				c = rh_getchar(ctx);
			}
		}
	} else {
		while ('0' <= c && c <= '9') {
			intval = intval * 10 + (c - '0');
			c = rh_getchar(ctx);
		}
	}
	if (c == '.') {
		c = rh_getchar(ctx);
		isDbl = 1; dblval = (long double) intval;
		double power = 10.0;
		while ('0' <= c && c <= '9') {
			dblval += (c - '0') / power;
			power *= 10.0;
			c = rh_getchar(ctx);
		}
	}
	if ((isHex && (c == 'P' || c == 'p')) ||
			(!isHex && (c == 'E' || c == 'e'))) {
		if (!isDbl) isDbl = 1, dblval = (long double) intval;
		c = rh_getchar(ctx);
		double power = c == '-' ? 0.1 : 10.0;
		if (c == '+' || c == '-') c = rh_getchar(ctx);
		if ('0' <= c && c <= '9') {
			int p = 0;
			do {
				p = p * 10 + (c - '0');
				c = rh_getchar(ctx);
			} while ('0' <= c && c <= '9');
			for (; p; p--) dblval *= power;
		}
	}
	int countUnsigned = 0, countLong = 0, size = 4, countFloat = 0, sign = 1;
	for (;;) {
		if (c == 'l' || c == 'L') countLong++;
		else if (c == 'u' || c == 'U') countUnsigned++;
		else if (c == 'f' || c == 'F') countFloat++;
		else break;
		c = rh_getchar(ctx);
	}
	if (c == '_' || ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || 
			('0' <= c && c <= '9') || (isDbl && (countLong + countFloat > 1 || countUnsigned)) ||
			(!isDbl && (countLong > 2 || countFloat || countUnsigned > 1))) {
		E_ERROR(ctx, "missing in flag.");
		while (c != ' ' && c != '\n' && c != '\0') c = rh_getchar(ctx);
	}
	if (isDbl) size = countFloat ? 4 : countLong ? 16 : 8;
	if (!isDbl) size = countLong == 2 ? 8 : 4, sign = !countUnsigned;
	rh_type *type = rh_init_type();
	type->kind = isDbl ? RHTYP_FLOATING : RHTYP_NUMERIC;
	type->size = size; type->sign = sign;
	token->variable = rh_init_variable(type);
	if (isDbl) {
		if (size == 16) (*(long double *) token->variable->memory) = dblval;
		if (size == 8) (*(double *) token->variable->memory) = (double) dblval;
		if (size == 4) (*(float *) token->variable->memory) = (float) dblval;
	} else {
		memcpy(token->variable->memory, &intval, size);
	}
	rh_ungetc(ctx, c);
}

void rh_next_token_literal(rh_context *ctx, char c, rh_token *token) {
	token->type = TYP_LITERAL;
	char a = c;
	int count = 0, cval = 0, hp = ctx->hp;
	rh_type *type = rh_init_type();
	type->kind = RHTYP_NUMERIC;
	type->size = 1; type->sign = 1;
	if (a == '"') {
		rh_type *type2 = rh_init_type();
		type2->kind = RHTYP_POINTER;
		type2->child = type;
		type2->size = rh_get_typesize(type2);
		type = type2;
	}
	while ((c = rh_getchar(ctx)) != a) {
		if (c == '\n' || c == '\0') {
			E_ERROR(ctx, "Literal reached end of line.");
			break;
		} else if (c == '\\') {
			c = rh_getchar(ctx);
			if      (c == 'n') c = '\n';
			else if (c == 't') c = '\t';
			else if (c == 'v') c = '\v';
			else if (c == 'b') c = '\b';
			else if (c == 'r') c = '\r';
			else if (c == 'f') c = '\f';
			else if (c == 'a') c = '\a';
			else if (c == 'x') {
				int val = 0;
				for (;;) {
					c = rh_getchar(ctx);
					if      ('0' <= c && c <= '9') val = val * 16 + (c - '0');
					else if ('a' <= c && c <= 'f') val = val * 16 + (c - 'a' + 10);
					else if ('A' <= c && c <= 'F') val = val * 16 + (c - 'A' + 10);
					else break;
				}
				rh_ungetc(ctx, c);
				c = (char) val;
			} else if ('0' <= c && c <= '9') {
				int val = 0;
				while ('0' <= c && c <= '7') {
					val = val * 8 + (c - '0');
					c = rh_getchar(ctx);
				}
				rh_ungetc(ctx, c);
				c = (char) val;
			}
		}
		if (a == '"') ctx->memory[hp + count] = c;
		else cval = (cval << 8) + c;
		count++;
	}
	if (a == '\'' && count > 1) type->size = 4;
	if (a == '\'' && count == 0) {
		E_ERROR(ctx, "char literal error");
	}
	rh_variable *var = rh_init_variable(type);
	if (a == '"') {
		ctx->memory[hp + count] = '\0';
		*((int *) var->memory) = hp;
		ctx->hp = hp + count + 1;
	} else {
		if (count > 1) *((int *) var->memory) = cval;
		else *((signed char *) var->memory) = (signed char) cval;
	}
	token->variable = var;
	c = rh_getchar(ctx);
	rh_ungetc(ctx, c);
}

void rh_next_token_symbol(rh_context *ctx, char c, rh_token *token) {
	char s[RH_TOKEN_MAXLEN];
	token->type = TYP_SYMBOL;
	int i, j, k = 0;
	for (i = 0; token_symbol_table[i] != 0; i++) {
		for (j = 0, k = 0; c && token_symbol_table[i][j] == c; j++) {
			s[k++] = c; c = rh_getchar(ctx);
		}
		if (token_symbol_table[i][j] == '\0') {
			s[k] = '\0';
			break;
		} else {
			s[k++] = c;
			while (k > 1) rh_ungetc(ctx, s[--k]);
			if (k == 1) c = *s;
			k = 0;
		}
	}
	if (!k) {
		s[0] = c;
		s[1] = '\0';
		c = rh_getchar(ctx);
	}
	token->text = rh_malloc_string(s);
	rh_ungetc(ctx, c);
}

rh_token *rh_next_token(rh_context *ctx) {
	char c = rh_getchar(ctx);
	while (c == ' ' || c == '\t' || c == '\n') c = rh_getchar(ctx);
	if (c == '\0') return NULL;
	rh_token *token = rh_init_token(ctx);
	if (c == '_' || ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z'))
		rh_next_token_ident(ctx, c, token);
	else if ('0' <= c && c <= '9')  
		rh_next_token_numdbl(ctx, c, token);
	else if (c == '\'' || c == '"') 
		rh_next_token_literal(ctx, c, token);
	else rh_next_token_symbol(ctx, c, token);
	return token;
}


