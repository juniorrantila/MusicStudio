#include <MainWindow.h>

int x11_error_handler(Display* display, XErrorEvent* error)
{
    char buf[1024];
    XGetErrorText(display, error->error_code, buf, sizeof(buf));
    fprintf(stderr, "%s\n", buf);
    if (error->error_code == BadWindow)
        return 0;
    __builtin_trap();
}
