#include <task.h>
#include <common.h>

#define TIME_SLICES_USEC 1000

static void in_scheduler_timer()
{
  
}

static void set_scheduler_timer(int usec)
{
}

void run_task(task_t *t)
{
  set_scheduler_timer(TIME_SLICES_USEC);
  while(1) {
    printf("Sleeping\n");
    wait_usec(10);
  };
}
