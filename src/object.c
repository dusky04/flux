#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, objectType)                                         \
  (type *)allocateObject(sizeof(type), objectType)

static Obj *allocateObject(size_t size, ObjType type) {
  Obj *object = (Obj *)reallocate(NULL, 0, size);
  object->type = type;

  // Insert the object at the head of LL
  object->next = vm.objects;
  vm.objects = object;
  return object;
}

static ObjString *allocateString(char *chars, int length, uint32_t hash) {
  ObjString *string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  string->length = length;
  string->chars = chars;
  string->hash = hash;
  return string;
}

// FNV - 1a Hash Function
static uint32_t hashString(const char *key, int length) {
  uint32_t hash = 2166136261u;
  for (int i = 0; i < length; i++) {
    hash ^= (uint8_t)key[i];
    hash *= 16777619;
  }
  return hash;
}

ObjString *takeString(char *chars, int length) {
  uint32_t hash = hashString(chars, length);
  return allocateString(chars, length, hash);
}

ObjString *copyString(const char *chars, int length) {
  uint32_t hash = hashString(chars, length);

  // need to terminate the string ourselves because the lexeme points at a range
  // of characters inside the monolithic source string and isn’t terminated.
  // +1 for null byte
  char *heapChars = ALLOCATE(char, length + 1);
  memcpy(heapChars, chars, length);
  heapChars[length] = '\0';
  return allocateString(heapChars, length, hash);
}

void printObject(Value value) {
  switch (GET_OBJ_TYPE(value)) {
  case OBJ_STRING:
    printf("%s", AS_CSTRING(value));
    break;
  }
}