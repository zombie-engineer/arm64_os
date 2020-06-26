#pragma once
#include <compiler.h>
#include <types.h>
#include <usb/usb_pid.h>
#include <completion.h>
#include <memory/static_slot.h>

#define DWC2_TRANSFER_STATUS_STARTED 0
#define DWC2_TRANSFER_STATUS_SPLIT_HANDLED 1

typedef struct dwc2_pipe_desc {
  union {
    struct {
      uint64_t device_address  :  7;
      uint64_t ep_address      :  4;
      uint64_t ep_type         :  2;
      uint64_t ep_direction    :  1;
      uint64_t speed           :  2;
      uint64_t max_packet_size : 11;
      uint64_t dwc_channel     :  3;
      uint64_t hub_address     :  7;
      uint64_t hub_port        :  7;
    };
    uint64_t raw;
  } u;
} dwc2_pipe_desc_t;

#define PIPE_DESC_INIT(__hcd_pipe, __type, __dir) {\
    .u = {\
      .device_address  = __hcd_pipe->address, \
      .ep_address      = __hcd_pipe->endpoint,\
      .ep_type         = __type,\
      .ep_direction    = __dir,\
      .speed           = __hcd_pipe->speed,\
      .max_packet_size = __hcd_pipe->max_packet_size,\
      .hub_address     = __hcd_pipe->ls_hub_address,\
      .hub_port        = __hcd_pipe->ls_hub_port\
    }\
  }

#define DECL_PIPE_DESC(__name, __hcd_pipe, __type, __dir) \
  dwc2_pipe_desc_t __name = PIPE_DESC_INIT(__hcd_pipe, __type, __dir)

struct dwc2_transfer_ctl {
  struct list_head STATIC_SLOT_OBJ_FIELD(dwc2_transfer_ctl);
  int status;
  int transfer_size;
  int split_start;
  uint64_t dma_addr_base;
  uint64_t dma_addr;
  completion_fn completion;
  void *completion_data;
};

struct dwc2_transfer_ctl *dwc2_transfer_ctl_allocate();
void dwc2_transfer_ctl_deallocate(struct dwc2_transfer_ctl *);

struct dwc2_channel {
  dwc2_pipe_desc_t pipe ALIGNED(8);
  struct dwc2_transfer_ctl *tc;
};

/*
 * Return value: length of a printed string
 */
int dwc2_pipe_desc_to_string(dwc2_pipe_desc_t desc, char *buf, int bufsz);

typedef enum {
  DWC2_STATUS_ACK = 0,
  DWC2_STATUS_NAK,
  DWC2_STATUS_NYET,
  DWC2_STATUS_STALL,
  DWC2_STATUS_TIMEOUT,
  DWC2_STATUS_ERR,
} dwc2_transfer_status_t;

dwc2_transfer_status_t dwc2_transfer_blocking(dwc2_pipe_desc_t pipe, void *buf, int bufsz, usb_pid_t *pid, int *out_num_bytes);

bool dwc2_is_split_enabled(dwc2_pipe_desc_t pipe);
void dwc2_transfer_completed_debug(struct dwc2_channel *c);
void dwc2_transfer_prepare(dwc2_pipe_desc_t pipe, void *addr, int transfer_size, usb_pid_t *pid);
int dwc2_transfer_start(dwc2_pipe_desc_t pipe);
int dwc2_transfer_recalc_next(struct dwc2_channel *c);

struct usb_hub_port_status;

int dwc2_init_channels();

int dwc2_set_log_level(int log_level);
int dwc2_get_log_level();

void dwc2_reset(void);
void dwc2_start_vbus(void);
void dwc2_set_ulpi_no_phy(void);
uint32_t dwc2_get_vendor_id(void);
uint32_t dwc2_get_user_id(void);
void dwc2_print_core_regs(void);

/*
 * dwc2 high speed interface
 */
typedef enum {
    DWC2_HS_I_UTMI = 1,
    DWC2_HS_I_ULPI = 2,
    DWC2_HS_I_UTMI_ULPI = 3
} dwc2_hs_iface;

static inline const char *dwc2_hs_iface_to_string(dwc2_hs_iface hs)
{
  switch(hs) {
    case DWC2_HS_I_UTMI: return "UTMI";
    case DWC2_HS_I_ULPI: return "ULPI";
    case DWC2_HS_I_UTMI_ULPI: return "UTMI_ULPI";
    default: return "UNKNOWN";
  }
}

dwc2_hs_iface dwc2_get_hs_iface(void);

/*
 * dwc2 full speed interface
 */
typedef enum {
  DWC2_FS_I_PHYSICAL0 = 0,
  DWC2_FS_I_DEDICATED = 1,
  DWC2_FS_I_PHYSICAL2 = 2,
  DWC2_FS_I_PHYSICAL3 = 3,
} dwc2_fs_iface;

static inline const char *dwc2_fs_iface_to_string(int fs)
{
  switch(fs) {
    case DWC2_FS_I_PHYSICAL0: return "PHYSICAL0";
    case DWC2_FS_I_DEDICATED: return "DEDICATED";
    case DWC2_FS_I_PHYSICAL2: return "physical2";
    case DWC2_FS_I_PHYSICAL3: return "PHYSICAL3";
    default: return "UNKNOWN";
  }
}

dwc2_fs_iface dwc2_get_fs_iface(void);

void dwc2_set_fsls_config(int enabled);

void dwc2_set_dma_mode(void);

typedef enum {
  DWC2_OP_MODE_HNP_SRP_CAPABLE = 0,
  DWC2_OP_MODE_SRP_ONLY_CAPABLE = 1,
  DWC2_OP_MODE_NO_HNP_SRP_CAPABLE	= 2,
  DWC2_OP_MODE_SRP_CAPABLE_DEVICE = 3,
  DWC2_OP_MODE_NO_SRP_CAPABLE_DEVICE = 4,
  DWC2_OP_MODE_SRP_CAPABLE_HOST = 5,
  DWC2_OP_MODE_NO_SRP_CAPABLE_HOST = 6
} dwc2_op_mode_t;

static inline const char *dwc2_op_mode_to_string(dwc2_op_mode_t mode)
{
#define __CASE(__case) case DWC2_OP_MODE_ ## __case: return #__case
  switch(mode) {
    __CASE(HNP_SRP_CAPABLE);
    __CASE(SRP_ONLY_CAPABLE);
    __CASE(NO_HNP_SRP_CAPABLE);
    __CASE(SRP_CAPABLE_DEVICE);
    __CASE(NO_SRP_CAPABLE_DEVICE);
    __CASE(SRP_CAPABLE_HOST);
    __CASE(NO_SRP_CAPABLE_HOST);
    default: return "UNKNOWN";
  }
#undef __CASE
}

dwc2_op_mode_t dwc2_get_op_mode(void);

typedef enum {
  DWC2_OTG_CAP_NONE,
  DWC2_OTG_CAP_SRP,
  DWC2_OTG_CAP_HNP_SRP,
} dwc2_otg_cap_t;

void dwc2_set_otg_cap(dwc2_otg_cap_t);

void dwc2_power_clock_off(void);

bool dwc2_is_ulpi_fs_ls_only(void);

typedef enum {
  DWC2_CLK_30_60MHZ = 0,
  DWC2_CLK_48MHZ = 1,
  DWC2_CLK_6MHZ = 2
} dwc2_clk_speed_t;

void dwc2_set_host_speed(dwc2_clk_speed_t);

void dwc2_set_host_ls_support(void);

void dwc2_set_otg_hnp(void);

void dwc2_tx_fifo_flush(int fifonum);

void dwc2_rx_fifo_flush(void);

bool dwc2_is_dma_enabled(void);

bool dwc2_is_port_pwr_enabled(void);

void dwc2_port_set_pwr_enabled(bool);

void dwc2_port_reset(void);

void dwc2_port_reset_clear(void);

void dwc2_setup_fifo_sizes(int rx_fifo_sz, int non_periodic_fifo_sz, int periodic_fifo_sz);

void dwc2_enable_interrupts(void);

void dwc2_enable_ahb_interrupts(void);

void dwc2_unmask_all_interrupts(void);

void dwc2_unmask_port_interrupts(void);

void dwc2_enable_channel_interrupts(void);

void dwc2_clear_all_interrupts(void);

void dwc2_dump_int_registers(void);

void dwc2_disable_ahb_interrupts(void);

void dwc2_dump_port_int(int port);

void dwc2_dump_internal_status_reg(void);

void dwc2_irq_cb(void);

typedef void(*port_status_changed_cb_t)(struct usb_hub_port_status*);

void dwc2_set_port_status_changed_cb(port_status_changed_cb_t cb);

void dwc2_channel_enable(int ch);

void dwc2_channel_disable(int ch);

typedef int dwc2_chan_id_t;

#define DWC2_INVALID_CHANNEL -1

dwc2_chan_id_t dwc2_channel_alloc();

void dwc2_channel_free(dwc2_chan_id_t);

struct dwc2_channel *dwc2_channel_get(dwc2_chan_id_t ch);

void dwc2_channel_set_priv(dwc2_chan_id_t ch, void *priv);

void *dwc2_channel_get_priv(dwc2_chan_id_t ch);

void dwc2_init(void);
