/* Copyleft (c) Str1ker, 2024
 * https://github.com/Str1ker17
 * All rights are horse-fucked. */

// ReSharper disable CppNonInlineFunctionDefinitionInHeaderFile
#pragma once

#ifndef WHOAMI
#define WHOAMI __FILE__
#endif

#include <windows.h>
#include <tchar.h>

#define countof(a) (sizeof(a) / sizeof((a)[0]))

#define STRINGIZE(s) STRINGIZE2(s)
#define STRINGIZE2(s) #s

#define MessageBox_Show(text) MessageBox(NULL, _T(text), _T(WHOAMI), MB_ICONEXCLAMATION)
#define MessageBox_Show2(caption, text) MessageBox(NULL, _T(text), _T(caption), MB_ICONEXCLAMATION)

static void MessageBox_ShowError(TCHAR *text) {
    TCHAR buf[256];
    DWORD rv = GetLastError();
    int printed = wsprintf(buf, _T("%s\r\n\r\nWindows Error %u - "), text, rv);
    FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        rv,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        buf + printed,
        countof(buf) - printed,
        NULL
    );
    MessageBox(NULL, buf, _T(WHOAMI), MB_ICONERROR);
}

#if defined(NDEBUG)

#pragma function(memset)
void *__cdecl memset(void *ptr, int c, size_t len) {
    BYTE *p = ptr;
    for (SIZE_T i = 0; i < len; ++i) {
        *p++ = (BYTE)c;
    }
    return ptr;
}

#pragma function(memcpy)
void *__cdecl memcpy(void *dst, const void *src, size_t size) {
    void *start = dst;
    BYTE *bdst = dst;
    CONST BYTE *bsrc = src;
    while (size > 0) {
        *bdst++ = *bsrc++;
        --size;
    }
    return start;
}

#endif

#if defined(__RESHARPER__)
    #if !defined(UNICODE)
[[rscpp::format(printf, 1, 2)]]
    #else
[[rscpp::format(wprintf, 1, 2)]]
    #endif
#endif
int lprintf(TCHAR *fmt, ...) {
    TCHAR buf[256];
    va_list v1;
    va_start(v1, fmt);
    int len = wvsprintf(buf, fmt, v1);
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), buf, len, NULL, NULL);
    va_end(v1);
    return len;
}

/* Hide some complexity. */
#define MessageBox_ShowError(textA) MessageBox_ShowError(_T(textA))
#define lprintf(fmtA, ...) lprintf(_T(fmtA), ##__VA_ARGS__)
