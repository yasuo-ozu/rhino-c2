#pragma once

/* defined in rh_misc.c */
void *rh_malloc(size_t size);
void rh_free(void *addr);
void *rh_malloc_string(char *s);

 /* defined in rh_file.c */
rh_file *rh_init_file_from_fp(FILE *fp);
rh_file *rh_init_file(char *fname, int searchLocal);
void rh_free_file(rh_file *file);

/* defined in rh_file.c */
char rh_getchar(rh_context *ctx);
void rh_ungetc(rh_context *ctx, int c);

/* defined in rh_token.c */
void rh_dump_token(rh_token *token);
rh_token *rh_next_token(rh_context *ctx);

/* defined in rh_variable.c */
rh_variable *rh_init_variable(rh_type *type);
void rh_dump_variable(rh_variable *var);
void rh_free_variable(rh_variable *var);
rh_variable *rh_search_variable(rh_context *ctx, char *ident);
rh_variable *rh_convert_variable(rh_context *ctx, rh_variable *var, rh_type *type, int force);
int rh_variable_to_int(rh_context *ctx, rh_variable *var, int *intval);

/* defined in rh_type.c */
rh_type *rh_init_type();
void rh_free_type(rh_type *type);
rh_type *rh_dup_type(rh_type *orig);
void rh_dump_type(rh_type *type);
int rh_get_typesize(rh_type *type);
int rh_variable_to_int(rh_context *ctx, rh_variable *var, int *intval);

/* defined in rh_execute.c */
int rh_execute(rh_context *ctx);

/* defined in rh_error.c */
void rh_error(rh_context *ctx, rh_error_type type, char *msg, ...);
void rh_dump_error(rh_context *ctx);


