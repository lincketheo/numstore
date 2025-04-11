#include "os/io.h"

// lseek
// open
//

xfd* xfd_open(const char* fname, ...) {
}

i64 xfd_size(xfd* f);

int xfd_extend(xfd* f, u32 bytes);

int stderr_out_io(const char* fmt, ...);

int stdout_out_io(const char* fmt, ...);
