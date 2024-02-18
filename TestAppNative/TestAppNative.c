/* ReSharper disable CppClangTidyClangDiagnosticCastFunctionTypeStrict */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <tchar.h>

#include "picocrt.h"

#define htobe16(x) ((((x) & 0xff) << 8) | ((x) >> 8))
#define be16toh(x) htobe16(x)

static void WSAErrorHandler(
    const int rv, const TCHAR *file, const int line
#if defined(VERBOSE)
    , const TCHAR *op
#endif
) {
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
    lprintf("%s:%d: rv = %d, WSA error %d - %s\r\n", file, line, rv, rc, s);
#if defined(VERBOSE)
    lprintf("    %s\r\n", op);
#endif
    LocalFree(s);
}

/* clang-format off */
#if !defined(VERBOSE)
#define wsacall(op) do { int rv = (op); if (rv < 0) { WSAErrorHandler(rv, _T(__FILE__), __LINE__); return; } } while (0)
#else
#define wsacall(op) do { int rv = (op); if (rv < 0) { WSAErrorHandler(rv, _T(__FILE__), __LINE__, #op); return; } } while (0)
#endif
/* clang-format on */

static void TestCase1(void) {
    SOCKET s;
    wsacall(s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));
    int broadcastEnable = 1;
    wsacall(setsockopt(s, SOL_SOCKET, SO_BROADCAST, (const char *)&broadcastEnable, sizeof(broadcastEnable)));

    SOCKADDR_IN sin = {0};
    sin.sin_family = AF_INET;
    sin.sin_addr.S_un.S_addr = 0xffffffffU;
    sin.sin_port = htobe16(62900);
    wsacall(sendto(s, (const char *)&s, sizeof(s), 0, (const SOCKADDR *)&sin, sizeof(sin)));

    int namelen = sizeof(sin);
    wsacall(getsockname(s, (SOCKADDR *)&sin, &namelen));

    lprintf(
        "Local IPv4 socket address is %d.%d.%d.%d:%u\n",
        sin.sin_addr.S_un.S_un_b.s_b1,
        sin.sin_addr.S_un.S_un_b.s_b2,
        sin.sin_addr.S_un.S_un_b.s_b3,
        sin.sin_addr.S_un.S_un_b.s_b4,
        be16toh(sin.sin_port)
    );
}

int __cdecl _tmain(void) {
    WSADATA wsaData;

    wsacall(WSAStartup(MAKEWORD(2, 2), &wsaData));

    {
        TestCase1();
    }

    wsacall(WSACleanup());

    lprintf("Press any key to continue ... ");
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    FlushConsoleInputBuffer(hStdin);
    while (TRUE) {
        INPUT_RECORD ir;
        DWORD read;
        if (ReadConsoleInput(hStdin, &ir, 1, &read)) {
            if (ir.EventType == KEY_EVENT && ir.Event.KeyEvent.bKeyDown) {
                break;
            }
        }
    }

    return 0;
}

/* Entrypoint is overriden for Release builds (no CRT at all) in project settings */
#if defined(NDEBUG)
void DECLSPEC_NORETURN WINAPI EntryPoint(void) { ExitProcess(_tmain()); }
#endif
