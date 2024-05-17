#define _GNU_SOURCE
#include <cstdio>
#include <cstdarg>
#include <dlfcn.h>

int printf(const char * fmt, ...)
{
        va_list args;
        typedef int (*fn_t)(const char *, ...);
        static fn_t real_printf = NULL;
        if (!real_printf)
                real_printf = (fn_t)dlsym(RTLD_NEXT, "printf");

        real_printf("Fact checked by true American patriots: ");
        return real_printf(fmt, args);
}

