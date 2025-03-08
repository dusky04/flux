#ifndef FLUX_VM_H
#define FLUX_VM_H

#include "chunk.h"
#include "common.h"
#include "table.h"

#define MAX_STACK_LIMIT 256

typedef struct {
  // Chunk currently being operated on
  Chunk *chunk;

  // Instruction pointer
  // always points to the next instruction
  // to be executed
  uint8_t *ip;

  // Stack based VM
  Value stack[MAX_STACK_LIMIT];
  Value *stackTop; // Points at the element just past the element
  // containing the top value on the stack

  // Array of all the objects created dynamically during runtime
  // Done to free all the objects
  Obj *objects;

  Table strings;
} VM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_RUNTIME_ERROR,
  INTERPRET_COMPILE_ERROR
} InterpretResult;

extern VM vm;

void initVM();
void freeVM();

InterpretResult interpret(const char *source);

// Stack operations
void push(Value value);
Value pop();

#endif