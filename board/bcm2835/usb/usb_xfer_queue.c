#include <drivers/usb/usb_xfer_queue.h>
#include <list.h>

DECL_STATIC_SLOT(struct usb_xfer_job, usb_xfer_jobs, 64);
DECL_STATIC_SLOT(struct usb_xfer_jobchain, usb_xfer_jobchains, 64);

struct usb_xfer_job *usb_xfer_job_alloc(void)
{
  struct usb_xfer_job *job;
  job = usb_xfer_jobs_alloc();
  return job;
}

void usb_xfer_job_free(struct usb_xfer_job* j)
{
  usb_xfer_jobs_release(j);
}

void usb_xfer_link_next(struct usb_xfer_job *j, struct usb_xfer_job *next)
{
  j->next = next;
}

struct usb_xfer_jobchain *usb_xfer_jobchain_alloc(void)
{
  struct usb_xfer_jobchain *jobchain;
  jobchain = usb_xfer_jobchains_alloc();
  return jobchain;
}

void usb_xfer_jobchain_free(struct usb_xfer_jobchain *jc)
{
  usb_xfer_jobchains_release(jc);
}

void usb_xfer_queue_init(void)
{
  STATIC_SLOT_INIT_FREE(usb_xfer_jobs);
  STATIC_SLOT_INIT_FREE(usb_xfer_jobchains);
}
