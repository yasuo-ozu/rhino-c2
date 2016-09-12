#pragma once


#ifdef DEBUG
#define E_DEBUG(msg,...)	fprintf(stderr,msg "\n",__VA_ARGS__+0)
#else
#define E_DEBUG(msg,...)	
#endif

#define E_INIT(error)	{if(setjmp((error)->jmpbuf)){ \
	fprintf(stderr, "*** Stop.\n"); \
	rh_error_proc(stderr, error);}}



#define E_FATAL(ctx,msg,...)	(rh_error(ctx,ETYPE_FATAL,	 "%s:%d " msg,__FILE__,__LINE__,__VA_ARGS__+0))
#define E_ERROR(ctx,msg,...)	(rh_error(ctx,ETYPE_ERROR,	 "%s:%d " msg,__FILE__,__LINE__,__VA_ARGS__+0))
#define E_WARNING(ctx,msg,...)	(rh_error(ctx,ETYPE_WARNING, "%s:%d " msg,__FILE__,__LINE__,__VA_ARGS__+0))
#define E_NOTICE(ctx,msg,...)	(rh_error(ctx,ETYPE_NOTICE,	 "%s:%d " msg,__FILE__,__LINE__,__VA_ARGS__+0))
#define E_INTERNAL(ctx,msg,...)	(rh_error(ctx,ETYPE_INTERNAL,"%s:%d " msg,__FILE__,__LINE__,__VA_ARGS__+0))
