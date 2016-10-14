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
				else if (strcmp(c, "interactive") == 0) ctx->flag |= RHFLAG_INTERACTIVE; 
				else {
					E_FATAL(ctx, "unrecognized option %s", c);
				}
			} else {
				if (*c == '\0') {
					ctx->flag |= RHFLAG_STDIN;
				}
				for (; *c != '\0'; c++) {
					if (*c == 'd') ctx->flag |= RHFLAG_DEBUG;
					else if (*c == 'i') ctx->flag |= RHFLAG_INTERACTIVE;
					else {
						E_FATAL(ctx, "unrecognized flag %c", *c);
					}

				}
			}
		} else {
			file = rh_init_file(ctx, c, 1);
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
		rh_file *lastFile, *file = rh_init_file_from_fp(ctx, stdin);
		if (ctx->file == NULL) ctx->file = file;
		else {
			for (lastFile = ctx->file; lastFile->next != NULL; )
				lastFile = lastFile->next;
			lastFile->next = file;
		}
	}
	if (!(ctx->flag & RHFLAG_INTERACTIVE) && ctx->file == NULL) {
		E_FATAL(ctx, "souce file not specified.");
	}

	ctx->memory = rh_malloc(ctx, RH_MEMORY_SIZE);
	ctx->hp = 0;
	ctx->sp = RH_MEMORY_SIZE;
	ctx->token = NULL;
	ctx->token_top = NULL;
	ctx->token_bottom = NULL;

	if (ctx->flag & RHFLAG_INTERACTIVE) {
		for (;;) {
			printf("\rrhino> ");
			rh_parse *ps = rh_parse_statement(ctx);
			if (ps == NULL) break;
			rh_dump_parse(ctx, ps);
		}
	}

	rh_free_file(ctx, ctx->file);

	if (ctx->error.count) {
		rh_dump_error(ctx);
	}

	return 0;
}

int main(int argc, char **argv) {
	return rh_main(argc, argv);
}


