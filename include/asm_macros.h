#define FUNC(name) \
    .globl name; name

#define LFUNC(name) \
    .local name; name
  
#define GLOBAL_VAR(name) \
    .globl name; name
