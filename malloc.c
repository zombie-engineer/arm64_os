#include <memory/malloc.h>
#include <compiler.h>

char *kernel_memory[16 * 1024 * 1024] SECTION(".kernel_memory");

void *kmalloc(size_t size)
{
  return 0;
}
