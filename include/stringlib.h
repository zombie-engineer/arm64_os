#pragma once

#include <types.h>


// This trick helps testing scenarios on a host machine, when we should 
#ifndef TEST_STRING
#define strcmp     _strcmp
#define strncmp    _strncmp
#define strlen     _strlen
#define strnlen    _strnlen
#define strcpy     _strcpy
#define strncpy    _strncpy
#define memset     _memset
#define memcpy     _memcpy
#define strtoll    _strtoll
#define sprintf    _sprintf
#define snprintf   _snprintf
#define vsprintf   _vsprintf
#define vsnprintf  _vsnprintf
#define isprint    _isprint
#define isdigit    _isdigit
#endif
#define isspace  _isspace


int _strcmp(const char *s1, const char *s2);

int _strncmp(const char *s1, const char *s2, size_t n);

int _strlen(const char* ptr);

int _strnlen(const char*, size_t n);

char * _strcpy(char *, const char *);

char * _strncpy(char *, const char *, size_t n);

void * _memset(void *dst, char value, size_t n);

void * _memcpy(void *dst, const void *src, size_t n);

long long int _strtoll(const char *str, char **endptr, int basis);

int _vsprintf(char *dst, const char *fmt, __builtin_va_list args);

int _vsnprintf(char *dst, size_t n, const char *fmt, __builtin_va_list args);

int _sprintf(char *dst, const char *fmt, ...);

int _snprintf(char *dst, size_t n, const char *fmt, ...);

int _isspace(char c);

int _isprint(char c);

int _isdigit(char c);

#define SKIP_WHITESPACES(ptr) for (;*ptr && _isspace(*ptr);++ptr)
#define SKIP_WHITESPACES_BOUND(ptr, end) for (;ptr < end && *ptr && _isspace(*ptr);++ptr)
#define SKIP_NONWHITESPACES_BOUND(ptr, end) for (;ptr < end && *ptr && !_isspace(*ptr); ++ptr)

