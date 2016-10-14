#pragma once

/* defined in rh_misc.c */
void *rh_malloc(rh_context *ctx, size_t size);
void rh_free(rh_context *ctx, void *addr);
char *rh_malloc_string(rh_context *ctx, char *s);

 /* defined in rh_file.c */
rh_file *rh_init_file_from_fp(rh_context *ctx,FILE *fp);
rh_file *rh_init_file(rh_context *ctx, char *fname, int searchLocal);
void rh_free_file(rh_context *ctx, rh_file *file);
char rh_getchar(rh_context *ctx);
void rh_ungetc(rh_context *ctx, int c);

/* defined in rh_token.c */
void rh_dump_token(rh_context *ctx, rh_token *token);
rh_token *rh_next_token(rh_context *ctx);

/* defined in rh_variable.c */
rh_variable *rh_init_variable(rh_context *ctx, rh_type *type);
rh_variable *rh_init_variable_local(rh_context *ctx, rh_type *type, int depth, int isStatic);
void rh_dump_variable(rh_context *ctx, rh_variable *var);
void rh_free_variable(rh_context *ctx, rh_variable *var);
rh_variable *rh_search_variable(rh_context *ctx, char *ident);
rh_variable *rh_convert_variable(rh_context *ctx, rh_variable *var, rh_type *type, int force);
int rh_variable_to_int(rh_context *ctx, rh_variable *var, int *intval);
void rh_assign_variable(rh_context *ctx, rh_variable *dest, rh_variable *src);

/* defined in rh_type.c */
rh_type *rh_init_type(rh_context *ctx);
void rh_free_type(rh_context *ctx, rh_type *type);
rh_type *rh_dup_type(rh_context *ctx, rh_type *orig);
void rh_dump_type(rh_type *type);
int rh_get_typesize(rh_type *type);
int rh_variable_to_int(rh_context *ctx, rh_variable *var, int *intval);

/* defined in rh_parse.c */
void rh_dump_parse(rh_context *ctx, rh_parse *ps);
rh_parse *rh_parse_statement(rh_context *ctx);
int rh_execute(rh_context *ctx);

/* defined in rh_error.c */
void rh_error(rh_context *ctx, rh_error_type type, char *msg, ...);
void rh_dump_error(rh_context *ctx);

#ifdef DEBUG
extern char *__unused_tmpval;
#define UNUSED(a,...)	((void)(0 && printf(__unused_tmpval,a,__VA_ARGS__+0)))
#else
#define UNUSED(a,...)
#endif


