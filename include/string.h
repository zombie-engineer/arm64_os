#pragma once
#include <types.h>

int strcmp(const char *s1, const char *s2);

int strncmp(const char *s1, const char *s2, size_t n);

int strlen(const char* ptr);

int strnlen(const char*, size_t n);

char * strcpy(char *, const char *);

char * strncpy(char *, const char *, size_t n);

void * memset(void *dst, char value, size_t n);

void * memcpy(void *dst, const void *src, size_t n);


