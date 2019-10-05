#include <exception.h>

void generate_exception()
{
  *(volatile long int*)(0xffffffffffffffff) = 1;
}
