#pragma once

#define LDEBUG
#define LLOG
#define LERROR

#if defined(LDEBUG) || defined(LLOG) || defined(LERROR)
#include <stdio.h>
#endif

#ifdef LDEBUG
#define fprintf_dbg(...) fprintf (stdout, __VA_ARGS__)
#else
#define fprintf_dbg(...)
#endif

#ifdef LLOG
#define fprintf_log(...) fprintf (stdout, __VA_ARGS__)
#else
#define fprintf_log(...)
#endif

#ifdef LERROR
#define fprintf_err(...) fprintf (stderr, __VA_ARGS__)
#else
#define fprintf_err(...)
#endif
