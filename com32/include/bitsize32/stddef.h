/*
 * bits32/stddef.h
 */

#define _SIZE_T
#if defined(__s390__) || defined(__hppa__) || defined(__cris__)
typedef unsigned long size_t;
#else
typedef unsigned int size_t;
#endif

#define _PTRDIFF_T
typedef signed int ptrdiff_t;
