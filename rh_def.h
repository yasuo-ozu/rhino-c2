#pragma once

typedef struct rh_context	rh_context;
typedef struct rh_file		rh_file;
typedef enum   rh_token_type rh_token_type;
typedef struct rh_token		rh_token;

/* initialized in rh_main.c */
struct rh_context {
	enum {
		RHFLAG_DEBUG = 1,
		RHFLAG_STDIN = 2
	} flag;
	rh_file *file;
};

/* initialized in rh_file.c */
struct rh_file {
	char *name;
	FILE *fp;
	char unget_buf[16];
	int unget_buf_top;
	int is_linehead;	// reffed in rh_getchar()
	rh_file *next;
	rh_file *parent;
};

/* initialized in rh_token.c */
enum rh_token_type {
	TYP_NULL = 0,
	// Genre : set by rh_next_token()
	TYP_SYMBOL, TYP_IDENT, TYP_LITERAL, TYP_KEYWORD
};
struct rh_token {
	enum rh_token_type type;
	char *text;
};



