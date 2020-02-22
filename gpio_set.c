#include <gpio_set.h>
#include <common.h>
#include <error.h>
#include <stringlib.h>

static gpio_set_t gpio_sets[32] = { 0 };

static int gpio_set_active_count = 0;
static int gpio_set_initialized = 0;
static DECL_SPINLOCK(gpio_set_lock);

static inline gpio_set_t *gpio_set_get_by_handle(gpio_set_handle_t handle)
{
  gpio_set_t *set = &gpio_sets[(int)handle];
  if (set->active)
    return 0;
  return set;
}

int gpio_set_request(gpio_set_mask_t setmask, const char *owner_key, gpio_set_handle_t *out_handle)
{
  int i;
  int ret;
  gpio_set_t *set;

  if (!gpio_set_initialized)
    return ERR_NOT_INIT;

  spinlock_cond_lock(&gpio_set_lock);

  if (strlen(owner_key) != GPIO_SET_OWNER_KEY_LEN) {
    ret = ERR_INVAL_ARG;
    goto out_unlock;
  }

  if (gpio_set_active_count == ARRAY_SIZE(gpio_sets)) {
    ret = ERR_NO_RESOURCE;
    goto out_unlock;
  }

  for (i = 0; i < ARRAY_SIZE(gpio_sets); ++i) {
    set = &gpio_sets[i];
    if (set->active && set->mask & setmask) {
      ret = ERR_BUSY;
      goto out_unlock;
    }
  }

  for (i = 0; i < ARRAY_SIZE(gpio_sets); ++i) {
    set = &gpio_sets[i];
    if (!set->active)
      break;
  }
  
  set->mask = setmask;
  memcpy(set->owner_key, owner_key, GPIO_SET_OWNER_KEY_BUF_SZ);
  set->active = 1;

  gpio_set_active_count++;
  ret = ERR_OK;

out_unlock:
  spinlock_cond_unlock(&gpio_set_lock);
  return ret;
}

int gpio_set_release(gpio_set_handle_t handle, const char *owner_key)
{
  int ret;

  spinlock_cond_lock(&gpio_set_lock);

  gpio_set_t *set = gpio_set_get_by_handle(handle);
  if (!set) {
    ret = ERR_NOT_FOUND;
    goto out_unlock;
  }

  set->active = 0;
  ret = ERR_OK;

out_unlock:
  spinlock_cond_unlock(&gpio_set_lock);
  return ret;
}

int gpio_set_get_num_owners(gpio_set_mask_t setmask)
{
  int i;
  gpio_set_t *set;
  int ret = 0;

  if (!gpio_set_initialized)
    return ERR_NOT_INIT;

  spinlock_cond_lock(&gpio_set_lock);
  for (i = 0; i < ARRAY_SIZE(gpio_sets); ++i) {
    set = &gpio_sets[i];
    if (set->mask & setmask)
      ret++;
  }

  spinlock_cond_unlock(&gpio_set_lock);
  return ret;
}

int gpio_set_get_owners(gpio_set_mask_t setmask, void *out_sets_buf, int out_sets_buf_sz)
{
  int i;
  gpio_set_t *set;
  int ret = 0;

  if (!gpio_set_initialized)
    return ERR_NOT_INIT;

  gpio_set_t *out_set = (gpio_set_t *)out_sets_buf;
  gpio_set_t *end_set = out_set + out_sets_buf_sz / sizeof(gpio_set_t);

  spinlock_cond_lock(&gpio_set_lock);
  for (i = 0; i < ARRAY_SIZE(gpio_sets); ++i) {
    if (out_set >= end_set)
      break;
    set = &gpio_sets[i];
    if (!(set->mask & setmask))
      continue;

    *out_set = *set;
    ret++;
    out_set++;
  }

  spinlock_cond_unlock(&gpio_set_lock);
  return ret;
}

int gpio_set_init()
{
  if (gpio_set_initialized)
    return ERR_FATAL;

  spinlock_init(&gpio_set_lock);
  memset(gpio_sets, 0, ARRAY_SIZE(gpio_sets));
  gpio_set_active_count = 0;
  gpio_set_initialized = 1;
  return ERR_OK;
}
