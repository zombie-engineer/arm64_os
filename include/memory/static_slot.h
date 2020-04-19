#pragma once
#include <list.h>

#define __STATIC_SLOT_LIST(name) name ## _array
#define __STATIC_SLOT_FREE(name) name ## _free
#define __STATIC_SLOT_ACTIVE(name) name ## _active

#define STATIC_SLOT_OBJ_FIELD(name) list_static_slot_ ## name

#define __DECL_STATIC_SLOT_OBJECTS(type, name, numslots)\
  static type __STATIC_SLOT_LIST(name)[numslots];\
  static LIST_HEAD(__STATIC_SLOT_FREE(name));\
  static LIST_HEAD(__STATIC_SLOT_ACTIVE(name))

#define __DECL_STATIC_SLOT_ALLOC(type, name)\
static type *name ## _alloc()\
{\
  type *obj;\
  obj = list_next_entry_or_null(&__STATIC_SLOT_FREE(name),\
    type, STATIC_SLOT_OBJ_FIELD(name));\
  if (obj)\
    list_move(&obj->STATIC_SLOT_OBJ_FIELD(name), &__STATIC_SLOT_ACTIVE(name));\
  return obj;\
}

#define __DECL_STATIC_SLOT_RELEASE(type, name)\
void name ##_release(type *obj)\
{\
  list_move(&obj->STATIC_SLOT_OBJ_FIELD(name), &__STATIC_SLOT_FREE(name));\
}

#define __DECL_STATIC_SLOT_FUNCTIONS(type, name)\
  __DECL_STATIC_SLOT_ALLOC(type, name);\
  __DECL_STATIC_SLOT_RELEASE(type, name);

#define DECL_STATIC_SLOT(type, name, numslots)\
  __DECL_STATIC_SLOT_OBJECTS(type, name, numslots);\
  __DECL_STATIC_SLOT_FUNCTIONS(type, name);

#define STATIC_SLOT_INIT_FREE(name)\
  do {\
    int __i;\
    memset(__STATIC_SLOT_LIST(name), 0, sizeof(__STATIC_SLOT_LIST(name)));\
    for (__i = 0; __i < ARRAY_SIZE(__STATIC_SLOT_LIST(name)); ++__i) {\
      typeof(__STATIC_SLOT_LIST(name)[0]) *p = &__STATIC_SLOT_LIST(name)[__i];\
      list_add(&p->STATIC_SLOT_OBJ_FIELD(name),&__STATIC_SLOT_FREE(name));\
    }\
  } while(0);
