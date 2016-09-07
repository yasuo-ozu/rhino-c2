#pragma once

#include <setjmp.h>

#ifdef DEBUG
#define E_DEBUG(msg,...)	fprintf(stderr,msg "\n",__VA_ARGS__+0)
#else
#define E_DEBUG(msg,...)	
#endif

#define E_INIT(error)	{if(setjmp((error)->jmpbuf)){ \
	fprintf(stderr, "*** Stop.\n"); \
	rh_error_proc(stderr, error);}}
