#pragma once

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif
#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

#define IS_POW2_OR_ZERO(x) (((x) & ((x)-1)) == 0)