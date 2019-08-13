#include "tags.h"
#include "common.h"

typedef struct tag {
  unsigned int size;
  unsigned short id;
  unsigned short magic;
  char data[0];
} __attribute__((packed)) tag_t;


void tags_print_cmdline()
{
  volatile unsigned int *tag_base_ptr = (volatile unsigned int *)0x100;
  volatile tag_t *tag = (volatile tag_t *)tag_base_ptr;
  while(1)
  {
    if ((tag->size & 0xff) == 0)
      break;
    if (tag->id == 9)
    {
      printf("0x%08x: tag cmdline=%s\n", *(volatile unsigned int*)(tag->data), (volatile const char*)tag->data);
      break;
    }
    else
    {
      tag = (volatile tag_t*)(((volatile unsigned int*)(tag)) + tag->size);
    }
  }
}
