#pragma once

/* defined in rh_misc.c */
void *rh_malloc(size_t size);
void rh_free(void *addr);

 /* defined in rh_file.c */
rh_file *rh_init_file_from_fp(FILE *fp);
rh_file *rh_init_file(char *fname);
void rh_free_file(rh_file *file);

/* defined in rh_file.c */
char rh_getchar(rh_context *ctx);

