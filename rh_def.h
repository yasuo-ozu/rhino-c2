#pragma once

typedef struct rh_context	rh_context;
typedef struct rh_file		rh_file;
typedef enum   rh_token_type rh_token_type;
typedef struct rh_token		rh_token;
typedef struct rh_type		rh_type;
typedef struct rh_variable	rh_variable;

/* initialized in rh_main.c */
struct rh_context {
	enum {
		RHFLAG_DEBUG = 1,
		RHFLAG_STDIN = 2
	} flag;
	rh_file *file;
	unsigned char *memory;
	int hp, sp;
	rh_variable *variable;
	rh_token *token[1024];
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
	TYP_SYMBOL, TYP_IDENT, TYP_LITERAL, TYP_KEYWORD,
	// Opecode: set
};
struct rh_token {
	enum rh_token_type type;
	char *text;
	rh_token *next;

	/* Set in rh_parse() */
	rh_token *child[3];
	int address;	///< Set when TYP_CALL
};

/* initialized in rh_type.c */
struct rh_type {
	enum {
		RHTYP_NULL, RHTYP_ARRAY, RHTYP_POINTER, RHTYP_NUMERIC, RHTYP_FLOATING
	} kind;
	int length;		///< Array length. Used when ARRAY
	rh_type *child;	///< Used when ARRAY || POINTER
	int size, sign;	///< Used when NUMERIC || FLOATING
};

/* initialized in rh_variable.c */
struct rh_variable {
	rh_token *token;	///< Assigned when table entry
	rh_type *type;
	unsigned char *memory;
	rh_variable *next;	///< Used when table entry
	int is_dynamic;		///< Set when *memory is malloced.
}


