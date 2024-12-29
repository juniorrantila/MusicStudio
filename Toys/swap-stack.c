#include <Async/Stack.h>

#include <stdlib.h>
#include <stdio.h>

static void run_with_stack(void* stack, int stack_size, void* user, void(*callback)(void*));

static void callback(void* foo)
{
    char address;
    printf("Hello %p\n", &address + (long)foo);
}

int main(void)
{
    int stack_size = 16 * 1024;
    void* new_stack = malloc(stack_size);
    callback((void*)0);
    run_with_stack(new_stack, stack_size, (void*)0, callback);
    callback((void*)0);
}

static void run_with_stack(void* stack, int stack_size, void* user, void(*callback)(void*))
{
    void* old_stack = async_stack_swap(stack + stack_size - 512);
    callback(user);
    async_stack_swap(old_stack);
}
