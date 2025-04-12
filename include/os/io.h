#pragma once

#include "common/types.h"

///////////////////////////////////// Allocation Routines
typedef struct rads_alloc rads_alloc;
struct rads_alloc{
  void *(*malloc)(size_t);
  void (*free)(void*);
  void *(*realloc)(void*,int);
  int (*init)(void);
  int (*shutdown)(void);
};

rads_alloc* get_rads_alloc(void);

///////////////////////////////////// File and File Routines
typedef struct rads_file rads_file;
struct rads_file {
  const struct rads_io_methods *methods;
};

typedef struct rads_io_methods rads_io_methods;
struct rads_io_methods {
  int (*close)(rads_file*);
  int (*read)(rads_file*, void* dest, int n, u64 offset);
  int (*write)(rads_file*, const void* src, int n, u64 offset);
  int (*truncate)(rads_file*, u64 size);
  int (*sync)(rads_file*, int flags);
  int (*size)(rads_file*, i64 *pSize);
  int (*lock)(rads_file*, int);
  int (*unlock)(rads_file*, int);
};

rads_io_methods* get_rads_io_methods(void);

///////////////////////////////////// OS Level functions
typedef struct rads_vfs rads_vfs;
struct rads_vfs {
  int (*open)(rads_vfs*, rads_file* dest, const char* fname, int flags);
  int (*close)(rads_vfs*);
  int (*delete)(rads_vfs*, const char *zName, int syncDir);
};

rads_vfs* get_rads_vfs(void);

///////////////////////////////////// Logger
typedef struct rads_logger rads_logger;
struct rads_logger {
  void (*log_trace)(const char* fmt, ...);
  void (*log_debug)(const char* fmt, ...);
  void (*log_info)(const char* fmt, ...);
  void (*log_warn)(const char* fmt, ...);
  void (*log_error)(const char* fmt, ...);
};

rads_logger* get_rads_logger(void);
