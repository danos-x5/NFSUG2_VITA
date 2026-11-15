// Every import of SPEED2.EXE bound the way the generated registration binds it (win32::<dll>::<name> behind
// win32::Wrapper), so a missing or mismatched shim fails the runtime build and not only the game build.
#include <lib/imports.h>
#include <winapi/wrapper.h>
#include <winapi/advapi32.h>
#include <winapi/d3d9.h>
#include <winapi/ddraw.h>
#include <winapi/dinput8.h>
#include <winapi/dsound.h>
#include <winapi/gdi32.h>
#include <winapi/kernel32.h>
#include <winapi/netapi32.h>
#include <winapi/shell32.h>
#include <winapi/shfolder.h>
#include <winapi/tapi32.h>
#include <winapi/user32.h>
#include <winapi/winmm.h>
#include <winapi/ws2_32.h>

// Import table order of SPEED2.EXE (252 functions).
#define NFSU_IMPORTS(X) \
    X(advapi32, RegSetValueExA) X(advapi32, RegQueryValueExA) X(advapi32, RegOpenKeyExA) X(advapi32, RegOpenKeyA) \
    X(advapi32, RegCreateKeyA) X(advapi32, RegCloseKey) \
    X(ddraw, DirectDrawCreate) \
    X(dinput8, DirectInput8Create) \
    X(dsound, DirectSoundCreate) \
    X(gdi32, SetTextColor) X(gdi32, CreateCompatibleDC) X(gdi32, CreateFontA) X(gdi32, CreateBitmap) \
    X(gdi32, SetBkMode) X(gdi32, DeleteDC) X(gdi32, ExtTextOutA) X(gdi32, SetBkColor) X(gdi32, GetPixel) \
    X(gdi32, DeleteObject) X(gdi32, SelectObject) X(gdi32, BitBlt) \
    X(kernel32, VirtualQuery) X(kernel32, IsBadCodePtr) X(kernel32, GetCurrentProcessId) X(kernel32, SetEndOfFile) \
    X(kernel32, SetStdHandle) X(kernel32, FlushFileBuffers) X(kernel32, GetStringTypeW) X(kernel32, GetStringTypeA) \
    X(kernel32, IsValidCodePage) X(kernel32, IsValidLocale) X(kernel32, EnumSystemLocalesA) \
    X(kernel32, CompareStringA) X(kernel32, CompareStringW) X(kernel32, SetEnvironmentVariableA) \
    X(kernel32, IsDebuggerPresent) X(kernel32, DebugBreak) X(kernel32, GetLocaleInfoW) \
    X(kernel32, GetExitCodeThread) X(kernel32, GetLocaleInfoA) X(kernel32, GetUserDefaultLCID) \
    X(kernel32, LCMapStringW) X(kernel32, LCMapStringA) X(kernel32, SetUnhandledExceptionFilter) \
    X(kernel32, GetCPInfo) X(kernel32, GetOEMCP) X(kernel32, GetACP) X(kernel32, HeapSize) X(kernel32, HeapReAlloc) \
    X(kernel32, HeapCreate) X(kernel32, HeapDestroy) X(kernel32, TlsAlloc) X(kernel32, SetLastError) \
    X(kernel32, TlsFree) X(kernel32, GetStartupInfoA) X(kernel32, SetHandleCount) \
    X(kernel32, GetEnvironmentStringsW) X(kernel32, FreeEnvironmentStringsW) X(kernel32, GetEnvironmentStrings) \
    X(kernel32, FreeEnvironmentStringsA) X(kernel32, UnhandledExceptionFilter) X(kernel32, GetStdHandle) \
    X(kernel32, IsBadWritePtr) X(kernel32, IsBadReadPtr) X(kernel32, GetLastError) \
    X(kernel32, QueryPerformanceFrequency) X(kernel32, QueryPerformanceCounter) X(kernel32, FindClose) \
    X(kernel32, FindNextFileA) X(kernel32, FindFirstFileA) X(kernel32, CreateDirectoryA) \
    X(kernel32, MultiByteToWideChar) X(kernel32, GetModuleFileNameA) X(kernel32, GetModuleHandleA) \
    X(kernel32, GetLongPathNameA) X(kernel32, SetErrorMode) X(kernel32, CloseHandle) X(kernel32, Process32Next) \
    X(kernel32, Process32First) X(kernel32, Sleep) X(kernel32, CreateToolhelp32Snapshot) X(kernel32, SuspendThread) \
    X(kernel32, ResumeThread) X(kernel32, GetDriveTypeA) X(kernel32, GetLogicalDrives) X(kernel32, GetVersionExA) \
    X(kernel32, OutputDebugStringA) X(kernel32, SetProcessAffinityMask) X(kernel32, SetThreadPriority) \
    X(kernel32, SetPriorityClass) X(kernel32, GetProcessAffinityMask) X(kernel32, GetThreadPriority) \
    X(kernel32, GetPriorityClass) X(kernel32, GetCurrentThread) X(kernel32, GetCurrentProcess) \
    X(kernel32, FreeLibrary) X(kernel32, GetProcAddress) X(kernel32, LoadLibraryA) X(kernel32, WideCharToMultiByte) \
    X(kernel32, GetSystemInfo) X(kernel32, IsProcessorFeaturePresent) X(kernel32, EnterCriticalSection) \
    X(kernel32, InterlockedExchange) X(kernel32, InitializeCriticalSection) X(kernel32, InterlockedCompareExchange) \
    X(kernel32, DeleteCriticalSection) X(kernel32, LeaveCriticalSection) X(kernel32, GetFullPathNameA) \
    X(kernel32, MapViewOfFile) X(kernel32, GetFileSize) X(kernel32, CreateFileMappingA) X(kernel32, CreateFileA) \
    X(kernel32, CreateFileW) X(kernel32, UnmapViewOfFile) X(kernel32, LockResource) X(kernel32, LoadResource) \
    X(kernel32, SizeofResource) X(kernel32, FindResourceA) X(kernel32, FindResourceW) X(kernel32, VirtualFree) \
    X(kernel32, VirtualAlloc) X(kernel32, lstrcmpiA) X(kernel32, DuplicateHandle) X(kernel32, GetCurrentThreadId) \
    X(kernel32, TerminateThread) X(kernel32, SleepEx) X(kernel32, GetTickCount) X(kernel32, CreateEventA) \
    X(kernel32, SetEvent) X(kernel32, WaitForSingleObject) X(kernel32, ResetEvent) X(kernel32, GetCurrentDirectoryA) \
    X(kernel32, WriteFile) X(kernel32, SetFilePointer) X(kernel32, DeleteFileA) X(kernel32, MoveFileA) \
    X(kernel32, ReadFile) X(kernel32, GetDiskFreeSpaceExA) X(kernel32, GlobalAlloc) X(kernel32, GlobalFree) \
    X(kernel32, VirtualProtect) X(kernel32, CreateThread) X(kernel32, CreateMutexA) X(kernel32, ReleaseMutex) \
    X(kernel32, HeapFree) X(kernel32, GetProcessHeap) X(kernel32, HeapAlloc) X(kernel32, WaitCommEvent) \
    X(kernel32, GetOverlappedResult) X(kernel32, SetCommMask) X(kernel32, WaitForMultipleObjects) \
    X(kernel32, PurgeComm) X(kernel32, SetCommState) X(kernel32, GetCommState) X(kernel32, SetCommConfig) \
    X(kernel32, GetCommConfig) X(kernel32, SetCommTimeouts) X(kernel32, SetupComm) X(kernel32, GetCommandLineA) \
    X(kernel32, RtlUnwind) X(kernel32, ExitProcess) X(kernel32, TerminateProcess) \
    X(kernel32, GetTimeZoneInformation) X(kernel32, GetSystemTimeAsFileTime) X(kernel32, RaiseException) \
    X(kernel32, ExitThread) X(kernel32, TlsSetValue) X(kernel32, TlsGetValue) X(kernel32, FileTimeToSystemTime) \
    X(kernel32, FileTimeToLocalFileTime) X(kernel32, GetDiskFreeSpaceA) X(kernel32, GetFileType) \
    X(kernel32, RemoveDirectoryA) \
    X(netapi32, Netbios) \
    X(shell32, ShellExecuteA) \
    X(shfolder, SHGetFolderPathA) \
    X(tapi32, lineAnswer) X(tapi32, lineMakeCall) X(tapi32, lineGetID) X(tapi32, lineClose) \
    X(tapi32, lineNegotiateAPIVersion) X(tapi32, lineOpen) X(tapi32, lineGetDevCaps) X(tapi32, lineInitialize) \
    X(tapi32, lineShutdown) \
    X(user32, ShowCursor) X(user32, GetCursorPos) X(user32, wvsprintfA) X(user32, MessageBoxA) \
    X(user32, GetSystemMetrics) X(user32, DefWindowProcA) X(user32, ReleaseCapture) X(user32, GetMessageA) \
    X(user32, DispatchMessageA) X(user32, PeekMessageA) X(user32, TranslateMessage) X(user32, LoadIconA) \
    X(user32, LoadCursorA) X(user32, RegisterClassExA) X(user32, AdjustWindowRect) X(user32, GetDesktopWindow) \
    X(user32, CreateWindowExA) X(user32, UpdateWindow) X(user32, SetCursor) X(user32, SetFocus) \
    X(user32, SetForegroundWindow) X(user32, ShowWindow) X(user32, SetWindowLongA) X(user32, DestroyWindow) \
    X(user32, BeginPaint) X(user32, EndPaint) X(user32, PostMessageA) X(user32, PostThreadMessageA) \
    X(user32, PostQuitMessage) X(user32, SetCapture) X(user32, GetClientRect) X(user32, GetWindowRect) \
    X(user32, SetRect) X(user32, UnregisterClassA) X(user32, SetActiveWindow) X(user32, GetForegroundWindow) \
    X(user32, wsprintfA) X(user32, IsIconic) \
    X(winmm, timeSetEvent) X(winmm, timeKillEvent) X(winmm, timeBeginPeriod) X(winmm, timeGetDevCaps) \
    X(winmm, timeGetTime) X(winmm, timeEndPeriod) \
    X(ws2_32, WSAGetLastError) X(ws2_32, WSAStartup) X(ws2_32, setsockopt) X(ws2_32, ioctlsocket) X(ws2_32, socket) \
    X(ws2_32, closesocket) X(ws2_32, shutdown) X(ws2_32, bind) X(ws2_32, connect) X(ws2_32, listen) \
    X(ws2_32, accept) X(ws2_32, sendto) X(ws2_32, send) X(ws2_32, recvfrom) X(ws2_32, recv) X(ws2_32, select) \
    X(ws2_32, getpeername) X(ws2_32, getsockname) X(ws2_32, gethostbyname) X(ws2_32, gethostname) \
    X(ws2_32, WSAIoctl) X(ws2_32, WSACleanup) \
    X(d3d9, Direct3DCreate9)

namespace win32
{

#define NFSU_IMPORT_ENTRY(dll, name) \
    { #dll, #name, &Wrapper<decltype(&dll::name), &dll::name>::stdcall },

static const ImportEntry s_imports[] = {
    NFSU_IMPORTS(NFSU_IMPORT_ENTRY)
};

const ImportEntry* importsBegin()
{
    return s_imports;
}

const ImportEntry* importsEnd()
{
    return s_imports + sizeof(s_imports) / sizeof(s_imports[0]);
}

}
