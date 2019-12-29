#pragma once

#define NSEC_PER_SEC 1000000000L
#define USEC_PER_SEC 1000000L
#define USEC_PER_MSEC 1000L
#define MSEC_PER_SEC 1000L

#define SEC_TO_USEC(x) (x * USEC_PER_SEC)
#define MSEC_TO_USEC(x) (x * USEC_PER_MSEC)
#define SEC_TO_MSEC(x) (x * MSEC_PER_SEC)

