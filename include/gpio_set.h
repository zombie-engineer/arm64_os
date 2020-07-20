#pragma once
#include <types.h>
#include <error.h>
#include <common.h>

typedef uint64_t gpio_set_mask_t;
typedef int gpio_set_handle_t;
#define GPIO_SET_INVALID_HANDLE -1

#define GPIO_SET_OWNER_KEY_BUF_SZ 16
#define GPIO_SET_OWNER_KEY_LEN (GPIO_SET_OWNER_KEY_BUF_SZ - 1)

#define DECL_GPIO_SET_KEY(name, key) static const char name[16] = key "\0"

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

static inline gpio_set_handle_t gpio_set_checked_request(gpio_set_mask_t pins, const char *owner_key)
{
  gpio_set_handle_t handle;
  if (gpio_set_request(pins, owner_key, &handle) != ERR_OK) {
    printf("Failed to claim ownership for gpio pin set: %x, owner:%s\n", pins, owner_key);
    return GPIO_SET_INVALID_HANDLE;
  }
  return handle;
}

static inline gpio_set_handle_t gpio_set_request_1_pins(int gpio_pin, const char *owner_key)
{
  gpio_set_mask_t pins;
  gpio_set_mask_clear(&pins);
  gpio_set_mask_add(&pins, gpio_pin);
  return gpio_set_checked_request(pins, owner_key);
}

static inline gpio_set_handle_t gpio_set_request_2_pins(int gpio_pin_1, int gpio_pin_2, const char *owner_key)
{
  gpio_set_mask_t pins;
  gpio_set_mask_clear(&pins);
  gpio_set_mask_add(&pins, gpio_pin_1);
  gpio_set_mask_add(&pins, gpio_pin_2);
  return gpio_set_checked_request(pins, owner_key);
}

static inline gpio_set_handle_t gpio_set_request_n_pins(int* pins, int num_pins, const char *owner_key)
{
  int i;
  gpio_set_mask_t pin_mask;
  gpio_set_mask_clear(&pin_mask);
  for (i = 0; i < num_pins; ++i)
    gpio_set_mask_add(&pin_mask, pins[i]);
  return gpio_set_checked_request(pin_mask, owner_key);
}
