#ifndef FLUX_TABLE_H
#define FLUX_TABLE_H

#include "value.h"

// Simple Key-Value pair
// Currently, only strings are hashed
typedef struct {
  ObjString *key;
  Value value;
} Entry;

// Load Factor = (count / capacity)
typedef struct {
  // currently used memory
  // no. of key-value pairs currently stored
  int count;

  // Total allocated size of the entries array
  int capacity;

  Entry *entries;
} Table;

void initTable(Table *table);
void freeTable(Table *table);
bool tableGet(Table *table, ObjString *key, Value *value);
bool tableSet(Table *table, ObjString *key, Value value);
bool tableDelete(Table *table, ObjString *key);
void tableAddAll(Table *from, Table *to);

#endif