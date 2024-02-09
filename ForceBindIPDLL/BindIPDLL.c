/* ReSharper disable CppClangTidyBugproneReservedIdentifier */
/* ReSharper disable CppClangTidyClangDiagnosticReservedMacroIdentifier */
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <winsock2.h>

/* Unreferenced formal parameter */
#pragma warning(disable: 4100)
/* Unsafe conversion between function types */
#pragma warning(disable: 4191)
/* Function selected for automatic inline expansion */
#pragma warning(disable: 4711)
/* Compiler will insert Spectre mitigation */
#pragma warning(disable: 5045)

#define IADDR_MAX 64
static DWORD ipAddr_GlobalVar;

static int SetupHooks(void) {
    TCHAR ipAddrFromEnvVar[IADDR_MAX];
    CHAR ipAddrString[IADDR_MAX];
    if (GetEnvironmentVariable(_T("FORCEDIP"), ipAddrFromEnvVar, sizeof(ipAddrFromEnvVar)) != 0 ||
        ipAddrFromEnvVar[0] == '\0') {
        return 0;
    }
#if !defined(UNICODE)
    lstrcpy(ipAddrString, ipAddrFromEnvVar);
#else
    WideCharToMultiByte(CP_UTF8, 0, ipAddrFromEnvVar, -1, ipAddrString, sizeof(ipAddrFromEnvVar), 0, 0);
#endif
    ipAddr_GlobalVar = inet_addr(ipAddrString);
    return 1;
}

int WINAPI DllMain(HANDLE hModule, DWORD reason, LPVOID reserved) {
    switch (reason) {
        case DLL_PROCESS_ATTACH: {
            MessageBox(NULL, "Hey", "Yes", MB_OK);
            return SetupHooks();
        }
        case DLL_THREAD_ATTACH:  /* NOLINT(bugprone-branch-clone) */
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH: {
            return 1;
        }
        default: {
            return 1;
        }
    }
}
