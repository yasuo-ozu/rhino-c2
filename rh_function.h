#pragma once

/* defined in rh_misc.c */
void *rh_malloc(size_t size);
void rh_free(void *addr);
void *rh_malloc_string(char *s);

 /* defined in rh_file.c */
rh_file *rh_init_file_from_fp(FILE *fp);
rh_file *rh_init_file(char *fname);
void rh_free_file(rh_file *file);

/* defined in rh_file.c */
char rh_getchar(rh_context *ctx);
void rh_ungetc(rh_context *ctx, int c);

/* defined in rh_token.c */
void rh_dump_token(rh_token *token);
rh_token *rh_next_token(rh_context *ctx);

/* defined in rh_variable.c */
rh_variable *rh_init_variable();
void rh_free_variable(rh_variable *var);

/* defined in rh_type.c */
rh_type *rh_init_type();
void rh_free_type(rh_type *type);
rh_type *rh_dup_type(rh_type *orig);
int rh_get_typesize(rh_type *type);
extern const RH_TYPE_INT32;
extern const RH_TYPE_UINT32;
extern const RH_TYPE_CHARPTR;


