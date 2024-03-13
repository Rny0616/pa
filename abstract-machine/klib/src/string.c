#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s)
{
    size_t len = 0;
    while (s != NULL && *s != '\0')
    {
        s++;
        len++;
    }
    return len;
}

char *strcat(char *dst, const char *src)
{
    char *s = dst;

    while (*dst != '\0')
    {
        dst++;
    }
    while (*src != '\0')
    {
        *dst++ = *src++;
    }

    *dst = '\0';
    return s;
}


char *strcpy(char *dst, const char *src)
{
    char *s = dst;
    while (*src != '\0')
    {
        *dst++ = *src++;
    }
    *dst = '\0';
    return s;
}

char *strncpy(char *dst, const char *src, size_t n)
{
    char *s = dst;
    for (int i = 0; i < n; i++)
    {
        *dst++ = *src++;
    }
    *dst = '\0';
    return s;
}

int strcmp(const char *s1, const char *s2)
{

    while (*s1 == *s2&& *s1 != '\0'&&*s2 != '\0')
    {
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
    for (int i = 0; i < n; i++)
    {
        if (*s1 != *s2)
        {
            return *s1 - *s2;
        }
        s1++;
        s2++;
    }
    return 0;
}

void *memmove(void *dst, const void *src, size_t n)
{
    void *ret = dst;
    if (dst < src)
    {
        while (n--) // 前->后
        {
            *(char *)dst = *(char *)dst;
            dst = (char *)dst + 1;
            src = (char *)src + 1;
        }
    }
    else
    {
        while (n--) // 后->前
        {
            *((char *)dst + n) = *((char *)src + n);
        }
    }
    return ret;
}

void *memcpy(void *out, const void *in, size_t n)
{
    while (n--) // 前->后
    {
        *(char *)out = *(char *)in;
        out = (char *)out + 1;
        in = (char *)in + 1;
    }
    return 0;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
    for (int i = 0; i < n; i++)
    {
        if (*(char *)s1 != *(char *)s2)
        {
            return *(char *)s1 - *(char *)s2;
        }
        s1++;
        s2++;
    }
    return 0;
}
void *memset(void *s, int c, size_t n)
{
    void *ss = s;
    for (int i = 0; i < n; i++)
    {
        *(char *)ss = c;
        ss++;
    }

    return s;
}

#endif
