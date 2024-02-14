/* ReSharper disable CppClangTidyClangDiagnosticCastFunctionTypeStrict */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <tchar.h>

int __cdecl _tmain(int argc, TCHAR *argv[], TCHAR *envp[]) {
    return 0;
}

/* Entrypoint is overriden for Release builds (no CRT at all) in project settings */
#if defined(NDEBUG)
int __cdecl EntryPoint(int argc, TCHAR *argv[], TCHAR *envp[]) {
    return _tmain(argc, argv, envp);
}
#endif
