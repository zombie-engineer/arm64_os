#pragma once

typedef struct cpu_ctx cpu_ctx_t;

int cpu_dump_ctx(void *ctx, char *buf, int bufsize);
