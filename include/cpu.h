#pragma once

typedef struct cpu_ctx cpu_ctx_t;

void cpu_dump_ctx(char *buf, int bufsize, void *ctx);
