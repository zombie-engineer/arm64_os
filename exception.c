#include <exception.h>
#include <stringlib.h>
#include <cpu.h>
#include <error.h>
#include <syscall.h>
#include <common.h>
#include <sched.h>

#define INTERRUPT_TYPE_SYNCHRONOUS 0
#define INTERRUPT_TYPE_IRQ         1
#define INTERRUPT_TYPE_FIQ         2
#define INTERRUPT_TYPE_SERROR      3

/*
 * Incomplete list of ARMv8 exception class types encoded into ELx_ECR register.
 * See ARM DDI0487B_b chapter D10.2.28
 */
#define EXC_CLASS_UNKNOWN           0b000000
#define EXC_CLASS_TRAPPED_WFI_WFE   0b000001
#define EXC_CLASS_ILLEGAL_EXECUTION 0b001110
#define EXC_CLASS_SVC_AARCH32       0b010001
#define EXC_CLASS_SVC_AARCH64       0b010101
#define EXC_CLASS_INST_ABRT_LO_EL   0b100000
#define EXC_CLASS_INST_ABRT_EQ_EL   0b100001
#define EXC_CLASS_INST_ALIGNMENT    0b100010
#define EXC_CLASS_DATA_ABRT_LO_EL   0b100100
#define EXC_CLASS_DATA_ABRT_EQ_EL   0b100101
#define EXC_CLASS_STACK_ALIGN_FLT   0b100110
#define EXC_CLASS_FLOATING_POINT    0b101100

/*
 * Data abort decoded values from ARM DDI0487B_b
 */
/* Address size fault */
#define DAT_ABRT_ADDR_SIZE                             0b000000
/* Translation fault */
#define DAT_ABRT_TRANSLATION                           0b000100
/* Access flag fault */
#define DAT_ABRT_ACCESS_FLAG                           0b001000
/* Permission fault */
#define DAT_ABRT_PERM                                  0b001100
/* Synchronous External abort, not table walk */
#define DAT_ABRT_SYNCH_EXT                             0b010000
/* Synchronous Tag Check */
#define DAT_ABRT_SYNCH_TAG_CHECK                       0b010001
/* Synchronous External abort, on table walk */
#define DAT_ABRT_SYNCH_EXT_TABLE_WALK                  0b010100
/* Synchronous parity or ECC error on memory access, not on table walk */
#define DAT_ABRT_SYNCH_MEMACCESS_PARITY_ECC            0b011000
/* Synchronous parity or ECC error on memory access, on table walk */
#define DAT_ABRT_SYNCH_MEMACCESS_PARITY_ECC_TABLE_WALK 0b011100
/* Alignment fault */
#define DAT_ABRT_ALIGNMENT                             0b100001
/* Unsup atomic hardware update fault */
#define DAT_ABRT_UNSUP_ATOMIC_HARDWARE_UPDATE          0b110001
/* TLB conflict abort */
#define DAT_ABRT_TLB_CONFLICT                          0b110000
/* Section Domain Fault */
#define DAT_ABRT_SECTION_DOMAIN                        0b111101
/* Page Domain Fault */
#define DAT_ABRT_PAGE_DOMAIN                           0b111110

static int exception_log_level = 0;
static const char *kernel_panic_msg = 0;

/* exception level to control nested entry to exception code */
static int elevel = 0;

/*
 * Fatal exception hooks can be set by kernel startup code as 
 * hook functions, which will be executed in case of an exception 
 * that was considered to be fatal / unrecoverable.
 * The system will execute all the hooks one after another before
 * entering a halted state.
 */
static exception_hook fatal_exception_hooks[8] = { 0 };
static int fatal_exception_hooks_count = 0;

/*
 * Kernel panic hooks are set by the system to provide control
 * of how panic messages are reported.
 */
static kernel_panic_reporter kernel_panic_reporters[8] = { 0 };
static int kernel_panic_reporters_count = 0;

/*
 * irq_cb / fiq_cb - callbacks set by kernel startup code 
 * that totally control the behavior of the system at triggered 
 * external interrupts.
 */
static irq_cb_t irq_cb = 0;
static irq_cb_t fiq_cb = 0;

void kernel_panic(const char *msg)
{
  kernel_panic_msg = msg;
  asm volatile ("svc #0x1000");
}

int add_kernel_panic_reporter(kernel_panic_reporter cb)
{
  if (kernel_panic_reporters_count == ARRAY_SIZE(kernel_panic_reporters))
    return ERR_NO_RESOURCE;

  kernel_panic_reporters[kernel_panic_reporters_count++] = cb;
  return ERR_OK;
}

static void exec_kernel_panic_reporters(exception_info_t *e, const char *msg)
{
  int i;
  for (i = 0; i < kernel_panic_reporters_count; ++i)
    kernel_panic_reporters[i](e, msg);
}

int add_unhandled_exception_hook(exception_hook cb)
{
  if (kernel_panic_reporters_count == ARRAY_SIZE(kernel_panic_reporters))
    return ERR_NO_RESOURCE;

  fatal_exception_hooks[fatal_exception_hooks_count++] = cb;
  return ERR_OK;
}

static void exec_fatal_exception_hooks(exception_info_t *e)
{
  int i;
  for (i = 0; i < fatal_exception_hooks_count; ++i) {
    printf("exec_fatal_exception_hooks: %p, %p\n", e, e->cpu_ctx);
    fatal_exception_hooks[i](e);
  }
}

void set_irq_cb(irq_cb_t cb)
{
  irq_cb = cb;
}

void set_fiq_cb(irq_cb_t cb)
{
  fiq_cb = cb;
}

static void __handle_data_abort(exception_info_t *e)
{
  exec_fatal_exception_hooks(e);
  while(1);
}

static void __handle_instr_abort(exception_info_t *e, uint64_t elr)
{
  exec_fatal_exception_hooks(e);
  while(1);
}

int get_synchr_exception_class(uint64_t esr)
{
  return (esr >> 26) & 0x7f; 
}

static void __handle_svc_32(exception_info_t *e)
{
}

#define get_svc_id(esr) (esr & 0xffff)

static void __handle_svc_64(exception_info_t *e)
{
  char buf[512];
  switch(get_svc_id(e->esr)) {
    case SVC_PANIC:
      exec_kernel_panic_reporters(e, kernel_panic_msg);
      // exec_fatal_exception_hooks(e);

      while(1)
        asm volatile ("wfe");

      break;
    case SVC_DO_NOTHING:
      break;
    case SVC_YIELD:
      schedule();
      break;
    default:
      snprintf(buf, sizeof(buf), "Unknown svc: %d", get_svc_id(e->esr));
      // puts(buf, 10, 9);
      break;
  }
}

static void __handle_interrupt_synchronous(exception_info_t *e)
{
  /* exception class */
  uint8_t ec;

  ec = get_synchr_exception_class(e->esr);

  switch (ec) {
    case EXC_CLASS_DATA_ABRT_LO_EL:
    case EXC_CLASS_DATA_ABRT_EQ_EL:
      __handle_data_abort(e);
      break;
    case EXC_CLASS_INST_ABRT_LO_EL:
    case EXC_CLASS_INST_ABRT_EQ_EL:
      __handle_instr_abort(e, e->elr);
      break;
    case EXC_CLASS_INST_ALIGNMENT:
      // puts("instruction alignment handler", 0, 0);
      break;
    case EXC_CLASS_SVC_AARCH64:
      __handle_svc_64(e);
      break;
    case EXC_CLASS_SVC_AARCH32:
      __handle_svc_32(e);
      break;
    default:
      break;
  }
}


static void __handle_interrupt_serror()
{
}

static void __handle_interrupt_irq()
{
  if (irq_cb)
    irq_cb();
}

const char *get_data_abort_string(uint64_t esr)
{
  int dfsc = esr & 0x0000003f;

#define SWICASE(x) case x: return #x
  switch(dfsc & 0x3c) {
    SWICASE(DAT_ABRT_ADDR_SIZE);
    SWICASE(DAT_ABRT_TRANSLATION);
    SWICASE(DAT_ABRT_ACCESS_FLAG);
    SWICASE(DAT_ABRT_PERM);
    SWICASE(DAT_ABRT_SYNCH_EXT);
    SWICASE(DAT_ABRT_SYNCH_TAG_CHECK);
    SWICASE(DAT_ABRT_SYNCH_EXT_TABLE_WALK);
    SWICASE(DAT_ABRT_SYNCH_MEMACCESS_PARITY_ECC);
    SWICASE(DAT_ABRT_SYNCH_MEMACCESS_PARITY_ECC_TABLE_WALK);
  }

  switch(dfsc) {
    SWICASE(DAT_ABRT_ALIGNMENT);
    SWICASE(DAT_ABRT_UNSUP_ATOMIC_HARDWARE_UPDATE);
    SWICASE(DAT_ABRT_TLB_CONFLICT);
    SWICASE(DAT_ABRT_SECTION_DOMAIN);
    SWICASE(DAT_ABRT_PAGE_DOMAIN);
  }

#undef SWICASE
  return "DAT_ABRT_UNDEF";
}

const char *get_exception_type_string(int type) 
{
  switch(type) {
    case INTERRUPT_TYPE_SYNCHRONOUS: return "Synchronous";
    case INTERRUPT_TYPE_IRQ        : return "IRQ";
    case INTERRUPT_TYPE_FIQ        : return "FIQ";
    case INTERRUPT_TYPE_SERROR     : return "SError";
    default                        : return "Undefined";
  }
}

const char *get_synchr_exception_class_string(uint64_t esr) 
{
  int exception_class = get_synchr_exception_class(esr);

  switch(exception_class) {
    case EXC_CLASS_UNKNOWN          : return "Unknown";
    case EXC_CLASS_TRAPPED_WFI_WFE  : return "Trapped WFI/WFE";
    case EXC_CLASS_ILLEGAL_EXECUTION: return "Illegal execution";
    case EXC_CLASS_SVC_AARCH32      : return "System call 32bit";
    case EXC_CLASS_SVC_AARCH64      : return "System call 64bit";
    case EXC_CLASS_INST_ABRT_LO_EL  : return "Instruction abort, lower EL";
    case EXC_CLASS_INST_ABRT_EQ_EL  : return "Instruction abort, same EL";
    case EXC_CLASS_INST_ALIGNMENT   : return "Instruction alignment fault";
    case EXC_CLASS_DATA_ABRT_LO_EL  : return "Data abort, lower EL";
    case EXC_CLASS_DATA_ABRT_EQ_EL  : return "Data abort, same EL";
    case EXC_CLASS_STACK_ALIGN_FLT  : return "Stack alignment fault";
    case EXC_CLASS_FLOATING_POINT   : return "Floating point";
    default                         : return "Unknown2";
  }
}

const char *get_svc_aarch64_string(int esr)
{
#define SWICASE(x) case SVC_## x: return "SVC_" #x

  switch(get_svc_id(esr)) {
    SWICASE(PANIC);
  }
  return "SYSCALL64_UNKNOWN";
#undef SWICASE
}

const char *get_synch_exception_detail_string(uint64_t esr) 
{
  int exception_class = get_synchr_exception_class(esr);
  switch(exception_class) {
    case EXC_CLASS_DATA_ABRT_LO_EL  :
    case EXC_CLASS_DATA_ABRT_EQ_EL  : return get_data_abort_string(esr);
    case EXC_CLASS_UNKNOWN          : return "";
    case EXC_CLASS_TRAPPED_WFI_WFE  : return "";
    case EXC_CLASS_ILLEGAL_EXECUTION: return "";
    case EXC_CLASS_SVC_AARCH32      : return "";
    case EXC_CLASS_SVC_AARCH64      : return get_svc_aarch64_string(esr);
    case EXC_CLASS_INST_ABRT_LO_EL  : return "";
    case EXC_CLASS_INST_ABRT_EQ_EL  : return "";
    case EXC_CLASS_INST_ALIGNMENT   : return "";
    case EXC_CLASS_STACK_ALIGN_FLT  : return "";
    case EXC_CLASS_FLOATING_POINT   : return "";
    default                         : return "";
  }
}

int gen_exception_string_irq(exception_info_t *e, char *buf, size_t bufsz)
{
  return 0;
}

int gen_exception_string_fiq(exception_info_t *e, char *buf, size_t bufsz)
{
  return 0;
}

int gen_exception_string_synch(exception_info_t *e, char *buf, size_t bufsz)
{
  return snprintf(buf, bufsz, "class: %s, detail: %s",
    get_synchr_exception_class_string(e->esr),
    get_synch_exception_detail_string(e->esr)
  );
}

int gen_exception_string_serror(exception_info_t *e, char *buf, size_t bufsz)
{
  return 0;
}

int gen_exception_string_specific(exception_info_t *e, char *buf, size_t bufsz)
{
  switch (e->type) {
    case INTERRUPT_TYPE_IRQ:         return gen_exception_string_irq(e, buf, bufsz);
    case INTERRUPT_TYPE_FIQ:         return gen_exception_string_fiq(e, buf, bufsz);
    case INTERRUPT_TYPE_SYNCHRONOUS: return gen_exception_string_synch(e, buf, bufsz);
    case INTERRUPT_TYPE_SERROR:      return gen_exception_string_serror(e, buf, bufsz);
  }
  return snprintf(buf, bufsz, "<UNKNOWN_TYPE>");
}

int gen_exception_string_generic(exception_info_t *e, char *buf, size_t bufsz)
{
  int n;
  n = snprintf( buf, bufsz, 
    "Excepiton: type:%s, esr: 0x%016llx, elr: 0x%016llx, far: 0x%016llx", 
    get_exception_type_string(e->type),
    e->esr,
    e->elr,
    e->far);
  return n;
}

void __handle_interrupt(exception_info_t *e)
{
  if (exception_log_level > 1)
    printf("__handle_interrupt: "__endline);

  elevel++;

  switch (e->type) {
    case INTERRUPT_TYPE_IRQ:
      __handle_interrupt_irq();
      break;
    case INTERRUPT_TYPE_FIQ:
      if (fiq_cb)
        fiq_cb();
      break;
    case INTERRUPT_TYPE_SYNCHRONOUS:
      __handle_interrupt_synchronous(e);
      break;
    case INTERRUPT_TYPE_SERROR:
      __handle_interrupt_serror();
      break;
  } 
  elevel--;
}

