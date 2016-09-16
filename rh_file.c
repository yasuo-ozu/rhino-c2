#include "rh_common.h"

rh_file *rh_init_file_from_fp(FILE *fp) {
	rh_file *file = rh_malloc(sizeof(rh_file));
	file->fp = fp;
	file->name = NULL;
	file->unget_buf_top = 0;
	file->next = NULL;
	file->parent = NULL;
	file->is_linehead = 1;
	return file;
}

rh_file *rh_init_file(char *fname, int searchLocal) {
	FILE *fp = NULL;
	rh_file *file = NULL;
	char buf[RH_FILENAME_MAXLEN];
	static char *include_dir[] = {
		"./include/", NULL
	};
	int i = 0;
	if (searchLocal) {
		fp = fopen(fname, "r");
	}
	while (fp == NULL && include_dir[i] != NULL) {
		strcpy(buf, include_dir[i]);
		strcpy(buf + strlen(buf), fname);
		fp = fopen(buf, "r");
		i++;
	}
	if (fp != NULL) {
		file = rh_init_file_from_fp(fp);
		file->name = rh_malloc(strlen(fname) + 1);
		strcpy(file->name, fname);
	}
	return file;
}

void rh_free_file(rh_file *file) {
	if (file == NULL) return;
	if (file->fp != stdin) fclose(file->fp);
	if (file->name != NULL) rh_free(file->name);
	rh_free(file);
}

void rh_ungetc(rh_context *ctx, int c) {
	if (c == -1) c = 0;
	ctx->file->unget_buf[ctx->file->unget_buf_top++] = (char) c;
	ctx->file->is_linehead = 0;
}

char rh_getc0(rh_context *ctx) {
	int c;
	rh_file *file = ctx->file;
	if (file->unget_buf_top)
		c = file->unget_buf[--file->unget_buf_top];
	else c = fgetc(file->fp);
	if (c == -1) c = 0;
	while (c == 0 && file->parent != NULL) {
		ctx->file = file->parent;
		rh_free_file(file);
		file = ctx->file;
		c = fgetc(file->fp);
	}
	FILE *fp = file->fp;
	if (c == '\\') {
		c = fgetc(fp);
		if (c != '?') {
			rh_ungetc(ctx, c);
			c = '\\';
		}
	} else if (c == '?') {
		c = fgetc(fp);
		if (c == '?') {
			c = fgetc(fp);
			if (c == '=') c = '#';
			else if (c == '(') c = '[';
			else if (c == '/') c = '\\';
			else if (c == ')') c = ']';
			else if (c == '\'') c = '^';
			else if (c == '<') c = '{';
			else if (c == '!') c = '|';
			else if (c == '>') c = '}';
			else if (c == '-') c = '~';
			else {
				rh_ungetc(ctx, c);
				rh_ungetc(ctx, '?');
				c = '?';
			}
		} else {
			rh_ungetc(ctx, c);
			c = '?';
		}
	}
	return (char) c;
}

char rh_getc(rh_context *ctx) {
	char c = rh_getc0(ctx);
	if (c == '\\') {
		c = rh_getc0(ctx);
		if (c == '\r' || c == '\n') {
			if (c == '\r') c = rh_getc0(ctx);
			if (c == '\n') c = rh_getc0(ctx);
		} else {
			rh_ungetc(ctx, c);
			c = '\\';
		}
	} 
	ctx->file->is_linehead = c == '\n';
	return c;
}

void rh_getchar_preprocess_include(rh_context *ctx, char *fname, char a) {
	rh_file *file = rh_init_file(fname, a == '"');
	if (file == NULL) {
		// TODO: open from /usr/include
		fprintf(stderr, "Include file open error: %s\n", fname);
		exit(1);
	}
	file->parent = ctx->file;
	ctx->file = file;
}

void rh_getchar_preprocess(rh_context *ctx) {
	char c = rh_getc(ctx), ident[16], *ch;
	while (c == ' ' || c == '\t') c = rh_getc(ctx);
	for (ch = ident; ('A' <= c && c <= 'Z') ||
			('a' <= c && c <= 'z'); ch++) {
		*ch = c;
		c = rh_getc(ctx);
	}
	*ch = '\0';
	if (strncmp(ident, "include", 15) == 0) {
		char a, fname[RH_FILENAME_MAXLEN];
		while (c != '\0' && c != '\n' && c != '<' && c != '"')
			c = rh_getc(ctx);
		a = c;
		if (a == '"' || a == '<') {
			a = a == '"' ? a : '>';
			ch = fname;
			c = rh_getc(ctx);
			while (c != a) {
				*ch++ = c; c = rh_getc(ctx);
			}
			*ch = '\0';
			while (c != '\n' && c != '\0') c = rh_getc(ctx);
			rh_ungetc(ctx, c);
			rh_getchar_preprocess_include(ctx, fname, a);
			
		} else {
			fprintf(stderr, "Include syntax error.\n");
			exit(1);
		}
	} else {
		fprintf(stderr, "Preprocessor command cannot be recognized.\n");
		exit(1);
	}
}

char rh_getchar(rh_context *ctx) {
	int is_linehead = ctx->file->is_linehead;
	char c = rh_getc(ctx);
	if (c == '/') {
		c = rh_getc(ctx);
		if (c == '/') {
			while (c != '\0' && c != '\n') c = rh_getc(ctx);
		} else if (c == '*') {
			do {
				c = rh_getc(ctx);
				if (c == '*') {
					c = rh_getc(ctx);
					if (c == '/') break;
				}
			} while (c != '\0');
			if (c == '\0') {
				fprintf(stderr, "Comment reached EOF.\n");
				exit(1);
			}
			c = ' ';
		} else {
			rh_ungetc(ctx, c);
			c = '/';
		}
	} else if (c == '#' && is_linehead) {
		rh_getchar_preprocess(ctx);
		c = ' ';
	}
	if (ctx->flag & RHFLAG_DEBUG) putchar(c);
	return c;
}


