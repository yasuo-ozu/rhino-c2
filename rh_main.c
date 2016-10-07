#include "rh_common.h"

void rh_read_options(rh_context *ctx, int argc, char **argv) {
	int i;
	char *c;
	rh_file *file, *nextFile = NULL;
	for (i = 1; i < argc; i++) {
		c = argv[i];
		if (*c == '-') {
			if (*++c == '-') {
				c++;
				if (strcmp(c, "dump") == 0) ctx->flag |= RHFLAG_DEBUG; 
				else if (strcmp(c, "interactive") == 0) ctx->flag |= RHFLAG_INTERACTIVE | RHFLAG_STDIN; 
				else {
					E_FATAL(ctx, "unrecognized option %s", c);
				}
			} else {
				if (*c == '\0') {
					ctx->flag |= RHFLAG_STDIN;
				}
				for (; *c != '\0'; c++) {
					if (*c == 'd') ctx->flag |= RHFLAG_DEBUG;
					else if (*c == 'i') ctx->flag |= RHFLAG_INTERACTIVE | RHFLAG_STDIN;
					else {
						E_FATAL(ctx, "unrecognized flag %c", *c);
					}

				}
			}
		} else {
			file = rh_init_file(c, 1);
			if (nextFile == NULL) ctx->file = file;
			else nextFile->next = file;
			nextFile = file;
		}
	}
}

int rh_main(int argc, char **argv) {
	
	rh_context context, *ctx = &context;
	
	ctx->file = NULL;
	ctx->flag = 0;
	ctx->error.count = 0;
	ctx->error.errors = 0;

	if (setjmp(ctx->error.jmpbuf)) {
		rh_dump_error(ctx);
		return (1);
	}

	rh_read_options(ctx, argc, argv);

	if (ctx->flag & RHFLAG_STDIN) {
		rh_file *lastFile, *file = rh_init_file_from_fp(stdin);
		if (ctx->file == NULL) ctx->file = file;
		else {
			for (lastFile = ctx->file; lastFile->next != NULL; )
				lastFile = lastFile->next;
			lastFile->next = file;
		}
	}
	if (ctx->file == NULL) {
		E_FATAL(ctx, "souce file not specified.");
	}

	ctx->memory = rh_malloc(RH_MEMORY_SIZE);
	ctx->hp = 0;
	ctx->sp = RH_MEMORY_SIZE;
	ctx->token = NULL;
	ctx->variable = NULL;
	ctx->variable_top = NULL;
	//ctx->depth = 0;

	if (!(ctx->flag & RHFLAG_INTERACTIVE)) {
		rh_token *token, *token_top = NULL;
		while ((token = rh_next_token(ctx)) != NULL) {
			if (ctx->flag & RHFLAG_DEBUG) {
				rh_dump_token(ctx, token);
			}
			if (token_top == NULL) {
				ctx->token = token;
			} else {
				token_top->next = token;
			}
			token_top = token;
		}
	}
	
	int ret = rh_execute(ctx);

	rh_free_file(ctx->file);

	if (ctx->error.count) {
		rh_dump_error(ctx);
	}

	return ret;
}

int main(int argc, char **argv) {
	return rh_main(argc, argv);
}


