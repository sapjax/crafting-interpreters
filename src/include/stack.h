#ifndef __STACK__H__
#define __STACK__H__

typedef void* item_type;
typedef struct _stack* stack;

stack stack_create();
void stack_destroy(stack s);

void stack_push(stack s, item_type elem);
item_type stack_pop(stack s);
item_type stack_peek(stack s, int i);
item_type stack_top(stack s);

int stack_is_empty(stack s);
int stack_size(stack s);
void stack_clear(stack s);
#endif  //!__STACK__H__