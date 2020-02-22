#pragma once
#include <types.h>

typedef uint64_t gpio_set_mask_t;
typedef int gpio_set_handle_t;

#define GPIO_SET_OWNER_KEY_BUF_SZ 16
#define GPIO_SET_OWNER_KEY_LEN (GPIO_SET_OWNER_KEY_BUF_SZ - 1)


typedef struct gpio_set {
  int active;
  char owner_key[GPIO_SET_OWNER_KEY_BUF_SZ];
  gpio_set_mask_t mask;
} gpio_set_t;

static inline void gpio_set_mask_clear(gpio_set_mask_t* mask) 
{
  *mask = 0;
}

static inline void gpio_set_mask_add(gpio_set_mask_t *mask, int gpio_num) 
{
  *mask |= (1<<gpio_num);
}

int gpio_set_request(gpio_set_mask_t setmask, const char *owner_key, gpio_set_handle_t *out_handle);

int gpio_set_release(gpio_set_handle_t handle, const char *owner_key);

int gpio_set_get_num_owners(gpio_set_mask_t setmask);

int gpio_set_get_owners(gpio_set_mask_t setmask, void *out_sets_buf, int out_sets_buf_sz);

int gpio_set_init();
