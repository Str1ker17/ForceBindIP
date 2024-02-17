/* ReSharper disable CppClangTidyClangDiagnosticCastFunctionTypeStrict */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <tchar.h>

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
    DWORD charsWritten;
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), buf, len, &charsWritten, NULL);
    va_end(v1);
    return len;
}

#define lprintf(fmt, ...) lprintf(_T(fmt), ##__VA_ARGS__)

static __declspec(noreturn) void WSAErrorHandler(const int rv, const TCHAR *file, const int line, const TCHAR *op) {
    int rc = WSAGetLastError();
    TCHAR *s;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&s,
        0,
        NULL
    );
    lprintf("%s:%d: WSA error %d - %s (rv = %d) when doing\r\n    %s\r\n", file, line, rc, s, rv, op);
    LocalFree(s);
    ExitProcess(1);
}

#define wsacall(op)                                                                                                            \
    do {                                                                                                                       \
        int rv = (op);                                                                                                         \
        if (rv < 0) {                                                                                                          \
            WSAErrorHandler(rv, L""__FILE__, __LINE__, L"" #op);                                                               \
        }                                                                                                                      \
    } while (0)

int __cdecl _tmain(void) {
    SOCKET s;
    WSADATA wsaData;

    wsacall(WSAStartup(MAKEWORD(2, 2), &wsaData));

    wsacall(s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));
    int broadcastEnable = 1;
    wsacall(setsockopt(s, SOL_SOCKET, SO_BROADCAST, (const char *)&broadcastEnable, sizeof(broadcastEnable)));

    SOCKADDR_IN sin = {0};
    sin.sin_family = AF_INET;
    sin.sin_addr.S_un.S_addr = 0xffffffffU;
    sin.sin_port = htons(62900);
    wsacall(sendto(s, (const char *)&s, sizeof(s), 0, (const SOCKADDR *)&sin, sizeof(sin)));

    int namelen = sizeof(sin);
    wsacall(getsockname(s, (SOCKADDR *)&sin, &namelen));

    lprintf(
        "Local IPv4 socket address is %d.%d.%d.%d:%u\n",
        sin.sin_addr.S_un.S_un_b.s_b1,
        sin.sin_addr.S_un.S_un_b.s_b2,
        sin.sin_addr.S_un.S_un_b.s_b3,
        sin.sin_addr.S_un.S_un_b.s_b4,
        ntohs(sin.sin_port)
    );

    wsacall(WSACleanup());

    lprintf("Press any key to continue ... ");
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
    while (TRUE) {
        INPUT_RECORD ir;
        DWORD read;
        if (ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &ir, 1, &read)) {
            if (ir.EventType == KEY_EVENT && ir.Event.KeyEvent.bKeyDown) {
                break;
            }
        }
    }

    return 0;
}

/* Entrypoint is overriden for Release builds (no CRT at all) in project settings */
#if defined(NDEBUG)
int WINAPI EntryPoint(void) { return _tmain(); }
#endif
