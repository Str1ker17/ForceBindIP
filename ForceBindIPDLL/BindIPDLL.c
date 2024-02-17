/* ReSharper disable CppClangTidyBugproneReservedIdentifier */
/* ReSharper disable CppClangTidyClangDiagnosticReservedMacroIdentifier */
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <winsock2.h>

#define countof(a) (sizeof(a) / sizeof((a)[0]))

#define IPADDR_MAX 64
static DWORD ipAddr_GlobalVar;

static int SetupHooks(void) {
    TCHAR ipAddrFromEnvVar[IPADDR_MAX];
    CHAR ipAddrString[IPADDR_MAX];
    if (GetEnvironmentVariable(_T("FORCEDIP"), ipAddrFromEnvVar, countof(ipAddrFromEnvVar) - 1) != 0 ||
        ipAddrFromEnvVar[0] == '\0') {
        return 0;
    }
#if !defined(UNICODE)
    lstrcpy(ipAddrString, ipAddrFromEnvVar);
#else
    WideCharToMultiByte(CP_UTF8, 0, ipAddrFromEnvVar, -1, ipAddrString, countof(ipAddrString), 0, 0);
#endif
    ipAddr_GlobalVar = inet_addr(ipAddrString);
    return 1;
}

int WINAPI DllMain(HANDLE hModule, DWORD reason, LPVOID reserved) {
    switch (reason) {
        case DLL_PROCESS_ATTACH: {
            MessageBox(NULL, _T("DllMain, DLL_PROCESS_ATTACH"), _T("BindIPDLL"), MB_OK);
            return SetupHooks();
        }
        case DLL_THREAD_ATTACH:  /* NOLINT(bugprone-branch-clone) */
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH: {
            return 1;
        }
        default: {
            MessageBox(NULL, _T("DllMain, unknown op"), _T("BindIPDLL"), MB_OK);
            return 0; /* F A I L */
        }
    }
}
