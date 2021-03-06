#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <stdarg.h>

#define RH_FILENAME_MAXLEN	2048
#define RH_TOKEN_MAXLEN		2048
#define RH_MEMORY_SIZE		4096	/* 4KB */

#define MAX(a,b)	((a)>(b)?(a):(b))
#define MIN(a,b)	((a)<(b)?(a):(b))

#include "rh_def.h"
#include "rh_function.h"
#include "rh_error.h"

