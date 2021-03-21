#include <node.h>
#include <node_object_wrap.h>
#include <uv.h>

//#include <crtdbg.h>

#ifndef assert
#define assert(ignore) ((void)0)
#endif

#ifndef DCHECK
#define DCHECK(x) assert(x)
#endif

// do {} while (0)
