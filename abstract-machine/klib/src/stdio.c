#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...)
{
  panic("Not implemented");
}

int vsprintf(char *out, const char *fmt, va_list ap)
{
  panic("Not implemented");
}

int sprintf(char *out, const char *fmt, ...)
{
    *out = '\0';
    va_list ap;
    int d;
    char *s;
    va_start(ap, fmt);
    while (*fmt)
    {
        if (*fmt == '%')
        {
            switch (*++fmt)
            {
            case 's': /* string */
                s = va_arg(ap, char *);
                strcat(out, s);
                fmt++;
                break;
            case 'd': /* int */
                d = va_arg(ap, int);
                char s[100] = {};
                strcat(out, itoa(d, s));
                fmt++;
                break;
            default:
                break;
            }
        }
        else
        {
            while (*out != '\0')
            {
                out++;
            }
            *out = *fmt;
            *(out+1) = '\0';
            fmt++;
        }
    }
    va_end(ap);
    return 0;
}

int snprintf(char *out, size_t n, const char *fmt, ...)
{
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap)
{
  panic("Not implemented");
}

#endif
