/* ReSharper disable CppClangTidyClangDiagnosticCastFunctionTypeStrict */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iphlpapi.h>
#include <tchar.h>

/* Unreferenced formal parameter */
#pragma warning(disable: 4100)
/* Unsafe conversion between function types */
#pragma warning(disable: 4191)
/* Function selected for automatic inline expansion */
#pragma warning(disable: 4711)
/* Compiler will insert Spectre mitigation */
#pragma warning(disable: 5045)

#define countof(a) (sizeof(a) / sizeof((a)[0]))

#define MessageBox_Show(text) MessageBox(NULL, _T(text), _T("ForceBindIP"), MB_ICONEXCLAMATION)
#define MessageBox_Show2(caption, text) MessageBox(NULL, _T(text), _T(caption), MB_ICONEXCLAMATION)

static void MessageBox_ShowError(LPTSTR text) {
    TCHAR buf[256];
    DWORD rv = GetLastError();
    int printed = wsprintf(buf, _T("%s\r\n\r\nWindows Error %u - "), text, rv);
    TCHAR *ptr = buf + printed;
    FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        rv,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        ptr,
        countof(buf) - printed,
        NULL
    );
    MessageBox(NULL, buf, _T("ForceBindIP"), MB_ICONERROR);
}

/* Hide some complexity. */
#define MessageBox_ShowError(text) MessageBox_ShowError(_T(text))

typedef _Success_(return != FALSE) BOOL(WINAPI *funcDecl_WriteProcessMemory)(
    _In_ HANDLE hProcess, _In_ LPVOID lpBaseAddress, _In_reads_bytes_(nSize) LPCVOID lpBuffer, _In_ SIZE_T nSize,
    _Out_opt_ SIZE_T *lpNumberOfBytesWritten
);

typedef _Ret_maybenull_ HANDLE(WINAPI *funcDecl_CreateRemoteThread)(
    _In_ HANDLE hProcess, _In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes, _In_ SIZE_T dwStackSize,
    _In_ LPTHREAD_START_ROUTINE lpStartAddress, _In_opt_ LPVOID lpParameter, _In_ DWORD dwCreationFlags,
    _Out_opt_ LPDWORD lpThreadId
);

typedef _Ret_maybenull_ HMODULE(WINAPI *funcDecl_LoadLibrary)(_In_ LPCSTR lpLibFileName);

static const CHAR funcName_CreateRemoteThread[] = "Zk|xm|K|tvm|Mqk|x}";
static const CHAR funcName_WriteProcessMemory[] = "Nkpm|Ikvz|jjT|tvk`";

#if !defined(UNICODE)
static const CHAR funcName_LoadLibrary[] = "LoadLibraryA";
#else
static const CHAR funcName_LoadLibrary[] = "LoadLibraryW";
#endif

#if defined(NDEBUG)
    #pragma function(memset)

void *__cdecl memset(void *ptr, int c, size_t len) {
    BYTE *p = ptr;
    for (SIZE_T i = 0; i < len; ++i) {
        *p++ = (BYTE)c;
    }
    return ptr;
}
#endif

PTCHAR lstrrchr(PTCHAR str, TCHAR c) {
    for (PTCHAR p = str + lstrlen(str); p >= str; --p) {
        if (*p == '\\') {
            return p;
        }
    }
    return NULL;
}

static void DecryptFunctionName(BYTE *data, BYTE *out) {
    while (*data != '\0') {
        *out++ = *data++ ^ 0x19U;
    }
    *out = '\0';
}

#define ResolveFunctionOnStack(hModule, funcName, funcType, funcAddr)                                                          \
    do {                                                                                                                       \
        CHAR funcNameDecrypted[sizeof(funcName)];                                                                              \
        DecryptFunctionName((BYTE *)(funcName), (BYTE *)funcNameDecrypted);                                                    \
        (funcAddr) = (funcType)GetProcAddress(hModule, funcNameDecrypted);                                                     \
        RtlZeroMemory(funcNameDecrypted, sizeof(funcNameDecrypted));                                                           \
    } while (0)

static int usage(void) { return MessageBox_Show2("Usage", "ForceBindIP usage"); }

#if !defined(UNICODE)
    #define WinMain WinMain
#else
    #define WinMain wWinMain
#endif

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, TCHAR *lpCmdLine, int nShowCmd) {
    HMODULE module_kernel32;
    funcDecl_WriteProcessMemory func_WriteProcessMemory;
    funcDecl_CreateRemoteThread func_CreateRemoteThread;
    funcDecl_LoadLibrary func_LoadLibrary;
    PROCESS_INFORMATION pInfo;
    STARTUPINFO StartupInfo;
    LPVOID remoteBuf;
    HANDLE remoteThread;
    DWORD creationFlags = 0;
    TCHAR dllName[MAX_PATH];
    IP_ADAPTER_INFO AdapterInfo[16];
    TCHAR ipAddrToBind[countof(_T("{92418c82-090a-433f-94ba-a0f99194b5c1}"))]; // adapter name or IP address

    dllName[0] = '\0';
    if (GetModuleFileName(NULL, dllName, countof(dllName)) == 0) {
        MessageBox_ShowError("ForceBindIP could not locate itself");
        return 1;
    }
    LPTSTR lastDelimiter;
    if ((lastDelimiter = lstrrchr(dllName, '\\')) == NULL) {
        MessageBox_Show("BindIP.dll cannot be found");
        return 1;
    }
    lstrcpy(lastDelimiter + 1, _T("BindIP.dll"));
    if (GetFileAttributes(dllName) == INVALID_FILE_ATTRIBUTES) {
        MessageBox_ShowError("ForceBindIP is not installed correctly: BindIP.dll not found.");
        return 1;
    }

    PTSTR ptr = lpCmdLine;

    BOOL delayedInjection = FALSE;
    if (*ptr == '-' && *(ptr + 1) == 'i') {
        ptr += countof(_T("-i"));
        delayedInjection = TRUE;
    }

    int idx = 0;
    while (TRUE) {
        if (*ptr == '\0' || *ptr == ' ') {
            break;
        }
        ipAddrToBind[idx++] = *ptr++;
    }
    ipAddrToBind[idx] = '\0';

    if (ipAddrToBind[0] - '0' < 0 || ipAddrToBind[0] - '0' > 9) {
        usage();
        return 1;
    }

    if (ipAddrToBind[0] == '{') {
        ULONG SizePointer = sizeof(AdapterInfo);
        if (GetAdaptersInfo(AdapterInfo, &SizePointer)) {
            MessageBox_ShowError("Error querying network adapters.");
            return 1;
        }
        PIP_ADAPTER_INFO p_AdapterInfo = &AdapterInfo[0];
        while (TRUE) {
            if (p_AdapterInfo == NULL) {
                MessageBox_Show("Couldn't find named adapter.");
                return 1;
            }
#if !defined(UNICODE)
            PTCHAR wAdapterName = p_AdapterInfo->AdapterName;
#else
            TCHAR wAdapterName[MAX_ADAPTER_NAME_LENGTH + 4];
            MultiByteToWideChar(CP_UTF8, 0, p_AdapterInfo->AdapterName, -1, wAdapterName, countof(wAdapterName));
#endif

            if (lstrcmp(ipAddrToBind, wAdapterName) == 0) {
#if !defined(UNICODE)
                lstrcpyn(ipAddrToBind, p_AdapterInfo->IpAddressList.IpAddress.String, sizeof(ipAddrToBind));
#else
                MultiByteToWideChar(
                    CP_UTF8, 0, p_AdapterInfo->IpAddressList.IpAddress.String, -1, ipAddrToBind, countof(ipAddrToBind)
                );
#endif
                break;
            }
            p_AdapterInfo = p_AdapterInfo->Next;
        }
    }

    if (*ptr == '\0') {
        usage();
        return 1;
    }

    while (*ptr == ' ') {
        ++ptr;
    }

    /* Pass to DLL. */
    SetEnvironmentVariable(_T("FORCEDIP"), ipAddrToBind);

    memset(&StartupInfo, 0, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);
    if (!delayedInjection) {
        creationFlags = CREATE_SUSPENDED;
    }
    memset(&pInfo, 0, sizeof(pInfo));
    if (!CreateProcess(NULL, ptr, NULL, NULL, TRUE, creationFlags, NULL, NULL, &StartupInfo, &pInfo)) {
        MessageBox_ShowError("ForceBindIP was unable to start the target program");
        return 1;
    }
    if (delayedInjection) {
        WaitForInputIdle(pInfo.hProcess, INFINITE);
    }

    /* Do the alchemy. */
    module_kernel32 = GetModuleHandle(_T("KERNEL32"));

    remoteBuf = VirtualAllocEx(pInfo.hProcess, NULL, 4096 /* page size on x86 */, MEM_COMMIT, PAGE_READWRITE);
    if (remoteBuf == NULL) {
        MessageBox_Show("Unable to allocate remote memory");
        return 1;
    }

    ResolveFunctionOnStack(module_kernel32, funcName_WriteProcessMemory, funcDecl_WriteProcessMemory, func_WriteProcessMemory);
    SIZE_T bytesWritten;
    if (func_WriteProcessMemory(pInfo.hProcess, remoteBuf, dllName, lstrlen(dllName), &bytesWritten) != TRUE) {
        MessageBox_Show("Unable to write remote memory");
        return 1;
    }

    ResolveFunctionOnStack(module_kernel32, funcName_CreateRemoteThread, funcDecl_CreateRemoteThread, func_CreateRemoteThread);
    func_LoadLibrary = (funcDecl_LoadLibrary)GetProcAddress(module_kernel32, funcName_LoadLibrary);
    remoteThread = func_CreateRemoteThread(
        pInfo.hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)func_LoadLibrary, remoteBuf, CREATE_SUSPENDED, NULL
    );
    if (remoteThread == INVALID_HANDLE_VALUE) {
        MessageBox_Show("Unable To Inject DLL");
        TerminateProcess(pInfo.hProcess, 1);
        return 1;
    }

    ResumeThread(remoteThread);
    if (WaitForSingleObject(remoteThread, INFINITE) != WAIT_OBJECT_0) {
        MessageBox_Show("Unable To Run DLL");
        TerminateProcess(pInfo.hProcess, 1);
        return 1;
    }

    if (!delayedInjection) {
        ResumeThread(pInfo.hThread);
    }

    CloseHandle(pInfo.hThread);
    CloseHandle(pInfo.hProcess);
    return 0;
}

/* Entrypoint is overriden for Release builds (no CRT at all) in project settings */
#if defined(NDEBUG)
int WINAPI EntryPoint(HINSTANCE hInstance, HINSTANCE hPrevInstance, TCHAR *lpCmdLine, int nShowCmd) {
    return WinMain(hInstance, hPrevInstance, lpCmdLine, nShowCmd);
}
#endif
