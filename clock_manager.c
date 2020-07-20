#include <clock_manager.h>
#include <common.h>
#include <reg_access.h>
#include <memory.h>
#include <stringlib.h>

#define CM_GP0CTL  (reg32_t)(PERIPHERAL_BASE_PHY + 0x00101070)
#define CM_GP1CTL  (reg32_t)(PERIPHERAL_BASE_PHY + 0x00101078)
#define CM_GP2CTL  (reg32_t)(PERIPHERAL_BASE_PHY + 0x00101080)
#define CM_PWMCTL  (reg32_t)(PERIPHERAL_BASE_PHY + 0x001010a0)

#define CM_GP0DIV  (reg32_t)(PERIPHERAL_BASE_PHY + 0x00101074)
#define CM_GP1DIV  (reg32_t)(PERIPHERAL_BASE_PHY + 0x0010107c)
#define CM_GP2DIV  (reg32_t)(PERIPHERAL_BASE_PHY + 0x00101084)
#define CM_PWMDIV  (reg32_t)(PERIPHERAL_BASE_PHY + 0x001010a4)


// Any value that is written into registers
// CLKGPn, CLKDIVn, CLKPWM, DIVPWM
// should be ORed with CM_REG_PASSWR
// *reg = 0x5a000000 | REGVALUE
// if that is not done, write operation is
// ignored and target register will still hold
// old value.
//
// Due to that reason declaration of helper
// struct is not feasable for these registers.

#define CM_REG_PASSWD          0x5a000000

#define CM_CLK_REG_SRC_GND     0
#define CM_CLK_REG_SRC_OSC     1
#define CM_CLK_REG_SRC_TSTDBG0 2
#define CM_CLK_REG_SRC_TSTDBG1 3
#define CM_CLK_REG_SRC_PLLA    4
#define CM_CLK_REG_SRC_PLLC    5
#define CM_CLK_REG_SRC_PLLD    6
#define CM_CLK_REG_SRC_HDMI    7

// Enable the clock generator
#define CM_CLK_REG_ENAB (1 << 4)

// Kill the clock generator
#define CM_CLK_REG_KILL (1 << 5)

// Readable flag to check that this clock control
// register is not writeable at the moment
// At BUSY you are only good for clearing
// CM_CLK_REG_ENAB flag.
#define CM_CLK_REG_BUSY (1 << 7)

#define CM_CLK_REG_FLIP (1 << 8)

#define CM_CTL_REG_ENABLE(regaddr) \
  *(reg32_t)regaddr = CM_REG_PASSWD | (*(reg32_t)regaddr | CM_CLK_REG_ENAB)

#define CM_CTL_REG_DISABLE(regaddr) \
  *(reg32_t)regaddr = CM_REG_PASSWD | (*(reg32_t)regaddr & ~CM_CLK_REG_ENAB)


#define CM_CLK_IS_BUSY(regaddr) (*(reg32_t)regaddr & CM_CLK_REG_BUSY)

#define CM_CLK_REG_SET(regaddr, clock_src, mash) \
  *(reg32_t)regaddr = CM_REG_PASSWD | clock_src | ((mash & 3) << 9)

#define CM_DIV_REG_SET(regaddr, divi, divf) \
  *(reg32_t)regaddr = CM_REG_PASSWD | ((divi & 0xfff) << 12) | (divf & 0xfff)

static int cm_clk_wait_nobusy(reg32_t cm_ctl, int cycles)
{
  do {
    if (!CM_CLK_IS_BUSY(cm_ctl))
      return CM_SETCLK_ERR_OK;
  } while(cycles--);
  return CM_SETCLK_ERR_BUSY;
}

int cm_set_clock(int clock_id, uint32_t clock_src, uint32_t mash, uint32_t divi, uint32_t divf)
{
  int st;
  reg32_t ctl;
  reg32_t div;

  switch(clock_id) {
    case CM_CLK_ID_GP_0:
      ctl = CM_GP0CTL;
      div = CM_GP0DIV;
      break;
    case CM_CLK_ID_GP_1:
      ctl = CM_GP1CTL;
      div = CM_GP1DIV;
      break;
    case CM_CLK_ID_GP_2:
      ctl = CM_GP2CTL;
      div = CM_GP2DIV;
      break;
    case CM_CLK_ID_PWM:
      ctl = CM_PWMCTL;
      div = CM_PWMDIV;
      break;
    default:
      return CM_SETCLK_ERR_INV;
  }

  // disable clock before any other writes
  CM_CTL_REG_DISABLE(ctl);

  // busy flag should go down now.
  st = cm_clk_wait_nobusy(ctl, 1000);
  if (st)
    return st;

  CM_CLK_REG_SET(ctl, clock_src, mash);
  CM_DIV_REG_SET(div, divi, divf);

  st = cm_clk_wait_nobusy(ctl, 1000);
  if (st)
    return st;

  CM_CTL_REG_ENABLE(ctl);
  return CM_SETCLK_ERR_OK;
}

const char *set_clock_err_to_str(int err)
{
  switch (err) {
    case CM_SETCLK_ERR_OK   : return "CM_SETCLK_ERR_OK";
    case CM_SETCLK_ERR_INV  : return "CM_SETCLK_ERR_INV";
    case CM_SETCLK_ERR_BUSY : return "CM_SETCLK_ERR_BUSY";
    default                 : return "UNKNOWN";
  }
}

struct clock_desc {
  reg32_t ctl;
  reg32_t div;
  const char *name;
};

static const char *src[] = {
  " GND",
  " OSC",
  "DBG0",
  "DBG1",
  "PLLA",
  "PLLC",
  "PLLD",
  "HDMI",
  " GND",
  " GND",
  " GND",
  " GND",
  " GND",
  " GND",
  " GND",
  " GND"
};

void cm_print_clock(int clock_id)
{
  uint32_t ctlval, divval;
  struct clock_desc *c;
#define CLK(x) { CM_ ## x ## CTL, CM_ ## x ## DIV, #x }
  struct clock_desc clocks[4] = {
    CLK(GP0),
    CLK(GP1),
    CLK(GP2),
    CLK(PWM)
  };

  if (clock_id > CM_CLK_ID_MAX) {
    puts("Clock id out of range\r\n");
    return;
  }

  c = &clocks[clock_id];
  ctlval = read_reg(c->ctl);
  divval = read_reg(c->div);

  printf("clock_manager:clock:%s:%08x:%08x:%s,%s,%s,MASH:%d,DIV:%d.%d\r\n",
      c->name,
      ctlval,
      divval,
      src[ctlval & 3],
      ((ctlval >> 4) & 1) ? "ENA" : "DIS",
      ((ctlval >> 7) & 1) ? "BUSY" : "STPD",
      ((ctlval >> 9) & 3),
      (divval >> 16) & 0xffff, divval & 0xffff
  );
}

void cm_print_clocks(void)
{
  int i;
  for (i = 0; i <= CM_CLK_ID_MAX; ++i)
    cm_print_clock(i);
}
