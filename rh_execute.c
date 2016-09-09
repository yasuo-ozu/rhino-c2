#include "common.h"

void rh_execute_token(rh_context *ctx, rh_token *token) {
	while (1) {
		if (token->type == TYP_EXIT) return;
		else if (token->type == TYP_JMP) {
			token = token->child[0]; continue;
		} else if (token->type == TYP_CALL) {
			ctx->sp -= 4;
			*((unsigned *)(ctx->memory + ctx->sp)) = token->address;
			token = token->child[0]; continue;
		} else if (token->type == TYP_RET) {
			unsigned ip = *((unsigned *)(ctx->memory + ctx->sp));
			ctx->sp += 4;
			token = ctx->token[ip];
			continue;
		} else if (token->type == TYP_CMP) {
			
		}
		token = token->next;
	}
}
