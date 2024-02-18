/* ReSharper disable CppClangTidyBugproneReservedIdentifier */
/* ReSharper disable CppClangTidyClangDiagnosticReservedMacroIdentifier */
// ReSharper disable once CppInconsistentNaming
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <winsock2.h>

#define WHOAMI "BindIP DLL"
#include "picocrt.h"

#define DECLARE_HOOK(module, func, hook)                                                                                       \
    { .moduleName = _T(module), .funcName = (func), .hookPtr = (UINT_PTR)(hook) }

typedef struct BindIP_HookData_S {
    TCHAR *moduleName;
    CHAR *funcName;
    LPVOID funcPtr;
    UINT_PTR hookPtr;
    BYTE origData[5];
    BYTE hookedData[5];
} BindIP_HookData;

int funcHook_sendto(SOCKET s, const SOCKADDR *name, int namelen) {
    SetLastError(WSAENETDOWN);
    return -1;
}

BindIP_HookData hookData[] = {
    DECLARE_HOOK("WS2_32", "sendto", funcHook_sendto),
};

#define IPADDR_MAX 64
static DWORD ipAddr_GlobalVar;

static int SetupHooks(void) {
    TCHAR ipAddrFromEnvVar[IPADDR_MAX];
    CHAR ipAddrString[IPADDR_MAX];

    if (GetEnvironmentVariable(_T("FORCEDIP"), ipAddrFromEnvVar, countof(ipAddrFromEnvVar)) == 0 ||
        ipAddrFromEnvVar[0] == '\0') {
        return 0;
    }

#if !defined(UNICODE)
    lstrcpy(ipAddrString, ipAddrFromEnvVar);
#else
    WideCharToMultiByte(CP_UTF8, 0, ipAddrFromEnvVar, -1, ipAddrString, countof(ipAddrString), 0, 0);
#endif

    ipAddr_GlobalVar = inet_addr(ipAddrString);

    for (unsigned i = 0; i < countof(hookData); ++i) {
        BindIP_HookData *d = &hookData[i];
        HANDLE hModule = GetModuleHandle(d->moduleName);
        if (hModule == NULL) {
            return FALSE;
        }
        LPVOID funcPtr = (LPVOID)GetProcAddress(hModule, d->funcName);
        if (ReadProcessMemory(GetCurrentProcess(), funcPtr, d->origData, sizeof(d->origData), NULL) != TRUE) {
            return FALSE;
        }

        d->funcPtr = funcPtr;
        UINT_PTR trampolineStart = (UINT_PTR)funcPtr + 5;
        UINT_PTR relativeOffset = d->hookPtr - trampolineStart;

        d->hookedData[0] = 0xE9; /* jmp */
        memcpy(&d->hookedData[1], &relativeOffset, sizeof(relativeOffset));
        if (WriteProcessMemory(GetCurrentProcess(), funcPtr, d->hookedData, sizeof(d->hookedData), NULL) != TRUE) {
            return FALSE;
        }
    }

    return TRUE;
}

/* 0 is failure, 1 is success */
int WINAPI DllMain(HANDLE hModule, DWORD reason, LPVOID reserved) {
    switch (reason) {
        case DLL_PROCESS_ATTACH: {
#if 0
            MessageBox(NULL, _T("DllMain, DLL_PROCESS_ATTACH"), _T("BindIPDLL"), MB_OK);
#endif
            return SetupHooks();
        }
        case DLL_THREAD_ATTACH: /* NOLINT(bugprone-branch-clone) */
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH: {
            return TRUE;
        }
        default: {
            MessageBox(NULL, _T("DllMain, unknown op"), _T("BindIPDLL"), MB_OK);
            return FALSE; /* F A I L */
        }
    }
}
