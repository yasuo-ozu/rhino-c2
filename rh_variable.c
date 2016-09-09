#include <stdio.h>

rh_variable *rh_init_variable() {
	rh_variable *var = rh_malloc(sizeof(rh_variable));
	var->token = NULL;
	var->type = NULL;
	var->memory = NULL;
	var->next = NULL;
	var->is_dynamic = NULL;
	return var;
}

void rh_free_variable(rh_variable *var) {
	if (var->is_dynamic && var->memory != NULL) {
		rh_free(var->memory);
	}
	rh_free(var);
}



