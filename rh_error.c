#include "rh_common.h"

#define ESC_ANSI_RED(s)		"\033[31m"s"\033[39m"
#define ESC_ANSI_GREEN(s)	"\033[32m"s"\033[39m"
#define ESC_ANSI_YELLOW(s)	"\033[33m"s"\033[39m"
#define ESC_ANSI_CYAN(s)	"\033[36m"s"\033[39m"

void rh_error(rh_context *ctx, rh_error_type type, char *msg, ...) {
	va_list va;
	char buf[1024], *c = buf;
	va_start(va, msg);
	;;;;;if (type == ETYPE_NOTICE)  strcpy(c, ESC_ANSI_CYAN		("INFO:     "));
	else if (type == ETYPE_WARNING) strcpy(c, ESC_ANSI_YELLOW	("WARNING:  "));
	else if (type == ETYPE_ERROR)   strcpy(c, ESC_ANSI_RED		("ERROR:    "));
	else if (type == ETYPE_FATAL)   strcpy(c, ESC_ANSI_RED		("FATAL:    "));
	else if (type == ETYPE_INTERNAL)strcpy(c, ESC_ANSI_RED		("INTERNAL: "));
	c += strlen(buf);
	vsnprintf(c, 1023 - strlen(buf), msg, va);
	ctx->error.messages[ctx->error.count++] = rh_malloc_string(buf);
	if (type == ETYPE_ERROR) ctx->error.errors++;
	if (type == ETYPE_INTERNAL || type == ETYPE_FATAL || ctx->error.errors >= 5) {
		longjmp(ctx->error.jmpbuf, 1);
	}
}

void rh_dump_error(rh_context *ctx) {
	fprintf(stderr, "\n\n\n");
	for (int i = 0; i < ctx->error.count; i++) {
		fprintf(stderr, "  %s\n", ctx->error.messages[i]);
	}
}
