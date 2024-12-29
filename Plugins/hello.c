[[clang::import_module("ms"), clang::import_name("info_puts")]]
void info_puts(char const*);

void ms_main(void) __asm__("ms_main");
void ms_main(void)
{
    info_puts("Hello, World!");
}
