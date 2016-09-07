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
				else {
					fprintf(stderr, "error: unrecognized option %s\n", c);
					exit(1);
				}
			} else {
				if (*c == '\0') {
					ctx->flag |= RHFLAG_STDIN;
				}
				for (; *c != '\0'; c++) {
					if (*c == 'd') ctx->flag |= RHFLAG_DEBUG;
					else {
						fprintf(stderr, "error: unrecognized flag %c\n", *c);
						exit(1);
					}

				}
			}
		} else {
			file = rh_init_file(c);
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
		fprintf(stderr, "error: souce file not specified.\n");
		exit(1);
	}

	rh_file *file;
	for (;;) {
		while (rh_getchar(ctx));
		file = ctx->file->next;
		if (file == NULL) break;
		rh_free_file(ctx->file);
		ctx->file = file;
	}
	rh_free_file(ctx->file);
	return 0;
}

int main(int argc, char **argv) {
	return rh_main(argc, argv);
}


