#include "include/stack.h"
#include <stdio.h>
#include <stdlib.h>

struct _stack {
  item_type* array;
  int max_size;
  int top;
};

static stack stack_create_maxsize(int max_size) {
  if (max_size <= 0) {
    fprintf(stderr, "Wrong stack size (%d)\n", max_size);
    abort();
  }
  stack s = (stack)malloc(sizeof(struct _stack));
  if (s == NULL) {
    fprintf(stderr, "Insufficient memory to initialize stack.\n");
    abort();
  }
  item_type* items;
  items = (item_type*)malloc(sizeof(item_type) * max_size);
  if (items == NULL) {
    fprintf(stderr, "Insufficient memory to initialize stack.\n");
    abort();
  }
  s->array = items;
  s->max_size = max_size;
  s->top = -1;
  return s;
}

stack stack_create() {
  return stack_create_maxsize(64);
}

void stack_destroy(stack s) {
  if (s == NULL) {
    fprintf(stderr, "Cannot destroy stack\n");
    abort();
  }
  free(s->array);
  free(s);
}

static void stack_doublesize(stack s) {
  if (s == NULL) {
    fprintf(stderr, "Cannot double stack size\n");
    abort();
  }
  // create double the stack
  int new_max_size = 2 * s->max_size;
  item_type* new_array = (item_type*)malloc(sizeof(item_type) * (new_max_size));
  if (new_array == NULL) {
    fprintf(stderr, "Insufficient memory to double the stack.\n");
    abort();
  }
  // copy elements to new array
  int i;
  for (i = 0; i <= s->top; i++) {
    new_array[i] = s->array[i];
  }
  // replace array with new array
  free(s->array);
  s->array = new_array;
  s->max_size = new_max_size;
}

void stack_push(stack s, item_type elem) {
  // increase capacity if necessary
  if (s->top >= s->max_size - 1) {
    stack_doublesize(s);
  }
  // push the element
  s->array[++s->top] = elem;
}

item_type stack_pop(stack s) {
  if (stack_is_empty(s)) {
    fprintf(stderr, "Can't pop element from stack: stack is empty.\n");
    abort();
  }
  item_type item = s->array[s->top];
  s->top--;
  return item;
}

item_type stack_top(stack s) {
  if (stack_is_empty(s)) {
    fprintf(stderr, "Stack is empty.\n");
    abort();
  }
  return s->array[s->top];
}

item_type stack_peek(stack s, int i) {
  if (stack_is_empty(s)) {
    fprintf(stderr, "Stack is empty.\n");
    abort();
  }
  if (i < 0 || i > s->top) {
    fprintf(stderr, "Index out of bounds.\n");
    abort();
  }
  return s->array[i];
}

int stack_is_empty(stack s) {
  if (s == NULL) {
    fprintf(stderr, "Cannot work with NULL stack.\n");
    abort();
  }
  return s->top < 0;
}
int stack_size(stack s) {
  if (s == NULL) {
    fprintf(stderr, "Cannot work with NULL stack.\n");
    abort();
  }
  return s->top + 1;
}

void stack_clear(stack s) {
  if (s == NULL) {
    fprintf(stderr, "Cannot work with NULL stack.\n");
    abort();
  }
  s->top = -1;
}
