#pragma once

int isspace(int c);

#define SKIP_WHITESPACES(ptr) for (;*ptr && isspace(*ptr);++ptr)
#define SKIP_WHITESPACES_BOUND(ptr, end) for (;ptr < end && *ptr && isspace(*ptr);++ptr)
#define SKIP_NONWHITESPACES_BOUND(ptr, end) for (;ptr < end && *ptr && !isspace(*ptr); ++ptr)
