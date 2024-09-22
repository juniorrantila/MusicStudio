#define createFoo(...) foo_args({ .args_check = 1, ##__VA_ARGS__ })
struct FooArgs {
    int args_check;
    int withWidth;
    int andHeight;
};
static void foo_impl(int width, int height);
void foo_args(FooArgs args);
void foo_args(FooArgs args)
{
    foo_impl(args.withWidth, args.andHeight);
}

static void foo_impl(int width, int height)
{
    (void)width;
    (void)height;
}

void bar();
void bar() {
    createFoo(1, 2);
    createFoo(withWidth: 0, andHeight: 1);
}
