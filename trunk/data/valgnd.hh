//
//  In order to avoid having valgrind generate more spurious warnings about
//  uninitialized memory, we modify the operation of some code to initialize
//  all memory used in system calls, even if the contents are not (hopefully)
//  reference in actual code - for example, we send some uninitialized bytes
//  over the network, our code never looks at them, but valgrind produces
//  a warning because the sendmsg call processes those bytes.
//
#ifndef Ami_valgnd_hh
#define Ami_valgnd_hh

#include <string.h>

//#ifdef VALGND
#if 1
#define strncpy_val(dst,src,sz) {		\
    if (src) {					\
      strncpy(dst,src,sz);			\
      if (strlen(src)<sz) memset(dst+strlen(src),0,sz-strlen(src));	\
    } else memset(dst,0,sz); }
#define memcpy_val(dst,src,sz,bsz) {		\
    memcpy(dst,src,sz);				\
    memset((char*)dst+sz,0,bsz-sz); }
#else
#define strncpy_val(dst,src,sz) { \
    if (src) strncpy(dst,src,sz); \
    else     dst[0]=0; }
#define memcpy_val(dst,src,sz,bsz) memcpy(dst,src,sz)
#endif

#endif
