#pragma once

#include <types.h>


// This trick helps testing scenarios on a host machine, when we should
#ifdef TEST_STRING
#define strcmp     _strcmp
#define strncmp    _strncmp
#define strlen     _strlen
#define strnlen    _strnlen
#define strcpy     _strcpy
#define strncpy    _strncpy
#define memset     _memset
#define memcpy     _memcpy
#define memcmp     _memcmp
#define strtoll    _strtoll
#define sprintf    _sprintf
#define snprintf   _snprintf
#define vsprintf   _vsprintf
#define vsnprintf  _vsnprintf
#define isprint    _isprint
#define isdigit    _isdigit
#endif
#define isspace  _isspace


int strcmp(const char *s1, const char *s2);

int strncmp(const char *s1, const char *s2, size_t n);

int strlen(const char* ptr);

int strnlen(const char*, size_t n);

char *strcpy(char *, const char *);

char *strncpy(char *, const char *, size_t n);

void *memset(void *dst, char value, size_t n);

void *memcpy(void *dst, const void *src, size_t n);

int memcmp(const void *a, const void *b, size_t n);

long long int strtoll(const char *str, char **endptr, int basis);

int vsprintf(char *dst, const char *fmt, __builtin_va_list *args);

int vsnprintf(char *dst, size_t n, const char *fmt, __builtin_va_list *args);

int sprintf(char *dst, const char *fmt, ...);

int snprintf(char *dst, size_t n, const char *fmt, ...);

int isspace(char c);

int isprint(char c);

int isdigit(char c);

void wtomb(char *buf, size_t buf_sz, char *src, int src_sz);

#define SKIP_WHITESPACES(ptr) for (;*ptr && _isspace(*ptr);++ptr)
#define SKIP_WHITESPACES_BOUND(ptr, end) for (;ptr < end && *ptr && _isspace(*ptr);++ptr)
#define SKIP_NONWHITESPACES_BOUND(ptr, end) for (;ptr < end && *ptr && !_isspace(*ptr); ++ptr)
