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
		RHFLAG_STDIN = 2,
		RHFLAG_INTERACTIVE = 3
	} flag;
	rh_file *file;
	unsigned char *memory;
	int hp, sp;
	rh_variable *variable, *variable_top;
	rh_token *token;
	int depth;
	struct {
		char *messages[20];
		int errors;			// counter of ETYPE_ERROR
		int count;			// counter of messages[]
		jmp_buf jmpbuf;
	} error;
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
	TYP_SYMBOL, TYP_IDENT, TYP_LITERAL, TYP_KEYWORD,
};
struct rh_token {
	enum rh_token_type type;
	char *text;
	rh_token *next;
	rh_variable *variable;

	rh_token *child[3];
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
	rh_token *token;	///< Assigned when table entry || func
	rh_type *type;
	unsigned char *memory;
	int address;		///< -1 when malloc()
	rh_variable *next;	///< Used when table entry
	int is_dynamic;		///< Set when *memory is malloced.
	int is_left;
	rh_variable *args;
	int args_count;		///< -1 when not a function
	rh_token *func_body;
};

/* initialized in rh_error.c */
typedef enum {
	ETYPE_NOTICE = 1, ETYPE_WARNING = 2, ETYPE_ERROR = 3,
	ETYPE_FATAL = 4,	/* error which stops compile immediately */
	ETYPE_TRAP = 5,		/* error which occurs in and stops executing */
	ETYPE_INTERNAL = 6	/* mainly thrown by rh_assert() */
} rh_error_type;



