#include <alloca.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "chunk.h"
#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vm.h"

VM vm;

static void resetStack() {
  // Point the SP at the start of the stack
  vm.stackTop = vm.stack;
}

static void runTimeError(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);

  fputs("\n", stderr);

  // Get the byte where the error occureed
  // Since the ip always points past the current intruction being executed
  // We do -1
  size_t instruction = vm.ip - vm.chunk->bytecode - 1;
  int line = vm.chunk->lines[instruction];
  fprintf(stderr, "[line %d] in script\n", line);
  resetStack();
}

void initVM() {
  resetStack();
  vm.objects = NULL;

  initTable(&vm.globals);
  initTable(&vm.strings);
}

void freeVM() {
  freeTable(&vm.globals);
  freeTable(&vm.strings);
  freeObjects();
}

void push(Value value) {
  // Dereference the SP and store the value at that addrese
  *(vm.stackTop) = value;
  vm.stackTop++;
}

Value pop() {
  // Since SP always points to the element past
  // the top element, so you decrement first
  // then dereference
  vm.stackTop--;
  return *(vm.stackTop);
}

static Value peek(int distance) { return vm.stackTop[-1 - distance]; }

static bool isFalsey(Value value) {
  // false and nil are falsey in flux like ruby
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate() {
  ObjString *b = AS_STRING(pop());
  ObjString *a = AS_STRING(pop());

  int length = a->length + b->length;
  char *chars = ALLOCATE(char, length + 1);
  memcpy(chars, a->chars, a->length);
  memcpy(chars + a->length, b->chars, b->length);
  chars[length] = '\0';

  ObjString *result = takeString(chars, length);
  push(OBJ_VAL(result));
}

static InterpretResult run() {
// Deference ip to get the bytecode and then increment the ip
// hence it always points to instruction about to be executed
#define READ_BYTE() (*(vm.ip++))

// For OP_CONSTANT, the next byte contains the index of the constant
// stored in the chunk
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])

// It reads a one-byte operand from the bytecode chunk. It treats that as an
// index into the chunk’s constant table and returns the string at that index
#define READ_STRING() AS_STRING(READ_CONSTANT())

#define BINARY_OP(valueType, op)                                               \
  do {                                                                         \
    if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {                          \
      runTimeError("Operands must be numbers.");                               \
    }                                                                          \
    double b = AS_NUMBER(pop());                                               \
    double a = AS_NUMBER(pop());                                               \
    push(valueType(a op b));                                                   \
  } while (false)

  while (true) {
#ifdef DEBUG_TRACE_EXECUTION
    printf("STACK: ");

    for (Value *slot = vm.stack; slot < vm.stackTop; slot++) {
      printf("[");
      printValue(*slot);
      printf("]");
    }
    printf("\n");

    // offset calculation turns it to a relative offset
    disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->bytecode));
#endif

    uint8_t instruction = READ_BYTE();
    switch (instruction) {
    case OP_CONSTANT: {
      Value constant = READ_CONSTANT();
      printf("Value pushed by OP_CONSTANT: ");
      printValue(constant);
      printf("\n");
      push(constant);
      break;
    }
    case OP_NIL:
      push(NIL_VAL);
      break;
    case OP_TRUE:
      push(BOOL_VAL(true));
      break;
    case OP_FALSE:
      push(BOOL_VAL(false));
      break;
    case OP_ADD:
      if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
        concatenate();
      } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
        double b = AS_NUMBER(pop());
        double a = AS_NUMBER(pop());
        push(NUMBER_VAL(a + b));
      } else {
        runTimeError("Operands must be two numbers or two strings");
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    case OP_SUBTRACT:
      BINARY_OP(NUMBER_VAL, -);
      break;
    case OP_MULTIPLY:
      BINARY_OP(NUMBER_VAL, *);
      break;
    case OP_DIVIDE:
      BINARY_OP(NUMBER_VAL, /);
      break;
    case OP_NOT:
      push(BOOL_VAL(isFalsey(pop())));
      break;
    case OP_EQUAL: {
      // Declaration immediately after a case <token> is not supported
      // in earlier versions of C
      Value b = pop();
      Value a = pop();
      push(BOOL_VAL(valuesEqual(a, b)));
      break;
    }
    case OP_GREATER:
      BINARY_OP(BOOL_VAL, >);
      break;
    case OP_LESS:
      BINARY_OP(BOOL_VAL, <);
      break;
    case OP_NEGATE:
      if (!IS_NUMBER(peek(0))) {
        runTimeError("Operand must be a number.");
        return INTERPRET_RUNTIME_ERROR;
      }
      push(NUMBER_VAL(-AS_NUMBER(pop())));
      break;
    case OP_RETURN:
      // printf("Value popped by OP_RETURN: ");
      // printValue(pop());
      // printf("\n");

      // Exit interpreter
      // Replaced by print right now
      return INTERPRET_OK;
    case OP_POP:
      pop();
      break;
    case OP_GET_LOCAL: {
      uint8_t slot = READ_BYTE();
      push(vm.stack[slot]);
      break;
    }
    case OP_DEFINE_GLOBAL: {
      // Get name of the variable from the constants table
      ObjString *name = READ_STRING();
      // Take value at top of the stack and store in a hash table with the name
      // as its key and value which is at the stack top
      tableSet(&vm.globals, name, peek(0));
      pop();
      break;
    }
    case OP_SET_LOCAL: {
      uint8_t slot = READ_BYTE();
      vm.stack[slot] = peek(0);
      break;
    }
    case OP_GET_GLOBAL: {
      ObjString *name = READ_STRING();
      Value value;
      if (!tableGet(&vm.globals, name, &value)) {
        runTimeError("Undefined variable '%s'.", name->chars);
        return INTERPRET_RUNTIME_ERROR;
      }
      // Push the value to the stack if the variable is defined previously
      push(value);
      break;
    }
    case OP_SET_GLOBAL: {
      ObjString *name = READ_STRING();
      // If the variable hasn't been declared previously, we through an error
      if (tableSet(&vm.globals, name, peek(0))) {
        tableDelete(&vm.globals, name);
        runTimeError("Undefined variable '%s'.", name->chars);
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }
    case OP_PRINT:
      printValue(pop());
      printf("\n");
      break;
    }
  }

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}

InterpretResult interpret(const char *source) {
  Chunk chunk;
  initChunk(&chunk);

  // Can't compile
  if (!compile(source, &chunk)) {
    freeChunk(&chunk);
    return INTERPRET_COMPILE_ERROR;
  }

  // The compiler will read the source
  // and populate the empty chunk with bytecode

  vm.chunk = &chunk;
  // Point IP towards the starting of the bytecode
  vm.ip = vm.chunk->bytecode;

  InterpretResult result = run();

  freeChunk(&chunk);

  return result;
}
