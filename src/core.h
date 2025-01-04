#ifndef _CORE_H
#define _CORE_H

#include <stdio.h>
#include "types.h"

#define DEBUG

#ifdef DEBUG
#define LOG(...) printf(__VA_ARGS__)
#define LOGF(...) printf(__VA_ARGS__); fflush(stdout)
#else
#define LOG(...)
#define LOGF(...)
#endif /* DEBUG */

void bitmap_print(u8 *bm);

#endif
