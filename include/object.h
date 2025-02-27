#ifndef FLUX_OBJECT_H
#define FLUX_OBJECT_H

#include "common.h"
#include "value.h"

#define GET_OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_STRING(value) isObjType(value, OBJ_STRING)

#define AS_STRING(value) ((ObjString *)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString *)AS_OBJ(value))->chars)

typedef enum {
  OBJ_STRING,
} ObjType;

struct Obj {
  ObjType type;

  // Linked List to keep track of all the objects created
  struct Obj *next;
};

struct ObjString {
  Obj Obj;
  int length;
  char *chars;

  // Hash value for the string
  // Immutable strings make sure this value never changes
  uint32_t hash;
};

ObjString *takeString(char *chars, int length);
ObjString *copyString(const char *chars, int length);
void printObject(Value value);

// Checks whether the given Obj is safe to downcast to the given type
static inline bool isObjType(Value value, ObjType type) {
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif