// Void DLL Injector
// GUI + process monitoring + injection settings
// All actions are triggered from the UI.
//
// Compile:
// rc resource.rc
// cl /EHsc /DUNICODE /D_UNICODE /Fe:void.exe updated.cpp resource.res user32.lib gdi32.lib comdlg32.lib shell32.lib comctl32.lib advapi32.lib

#include <windows.h>
#include <commdlg.h>
#include <tlhelp32.h>
#include <string>
#include <vector>
#include <shellapi.h>

// Resources
#include "resource.h"

// Mouse helpers
#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "advapi32.lib")

// Theme colors
static COLORREF BG       = RGB(2, 4, 10);
static COLORREF CARD     = RGB(7, 12, 25);
static COLORREF CARD2    = RGB(10, 22, 48);
static COLORREF BORDER   = RGB(25, 105, 235);
static COLORREF TEXT     = RGB(244, 246, 250);
static COLORREF MUTED    = RGB(145, 155, 175);
static COLORREF GREEN    = RGB(30, 205, 125);
static COLORREF CYAN     = RGB(0, 145, 255);
static COLORREF RED      = RGB(215, 80, 80);
static COLORREF YELLOW   = RGB(235, 177, 72);
static COLORREF ORANGE   = RGB(255, 165, 0);
static COLORREF HOVER    = RGB(18, 58, 125);

// Defaults
static const COLORREF DEFAULT_BG     = RGB(2, 4, 10);
static const COLORREF DEFAULT_CARD   = RGB(7, 12, 25);
static const COLORREF DEFAULT_CARD2  = RGB(10, 22, 48);
static const COLORREF DEFAULT_BORDER = RGB(25, 105, 235);
static const COLORREF DEFAULT_TEXT   = RGB(244, 246, 250);
static const COLORREF DEFAULT_MUTED  = RGB(145, 155, 175);
static const COLORREF DEFAULT_GREEN  = RGB(30, 205, 125);
static const COLORREF DEFAULT_CYAN   = RGB(0, 145, 255);
static const COLORREF DEFAULT_RED    = RGB(215, 80, 80);
static const COLORREF DEFAULT_YELLOW = RGB(235, 177, 72);
static const COLORREF DEFAULT_ORANGE = RGB(255, 165, 0);
static const COLORREF DEFAULT_HOVER  = RGB(18, 58, 125);

static HWND g_window = nullptr;
static HFONT g_titleFont = nullptr;
static HFONT g_bigFont = nullptr;
static HFONT g_bodyFont = nullptr;
static HFONT g_smallFont = nullptr;
static HBRUSH g_bgBrush = nullptr;

static wchar_t g_dllPath[MAX_PATH] = L"";
static wchar_t g_statusText[128] = L"Ready";
static DWORD g_pid = 0;
static bool g_isInjecting = false;
static bool g_wasRunning = false;
static bool g_alwaysOnTop = false;
static bool g_autoInject = false;
static int g_injectDelay = 8;
static bool g_minimizeToTray = false;
static bool g_startWithWindows = false;
static bool g_startMinimized = false;
static bool g_startWithMinecraft = false;
static bool g_isTray = false;
static bool g_isCustomTheme = false;
static bool g_monitorMinecraft = false;
static bool g_minecraftWasClosedByUser = false;

enum class AppState
{
    Ready,
    Working,
    Success,
    Error
};

static AppState g_state = AppState::Ready;
static int g_hover = 0;
static bool g_mouseTracking = false;
static NOTIFYICONDATAW g_trayData = {};

// Forward declarations
static void SetStatus(AppState state, const wchar_t* text);
static void UpdateStatus();
static void ToggleAlwaysOnTop();
static void ToggleAutoInject();
static void SetInjectDelay(int seconds);
static void ToggleMinimizeToTray();
static void ToggleStartWithWindows();
static void ToggleStartMinimized();
static void ToggleStartWithMinecraft();
static void ApplyTheme();
static void MinimizeToTray();
static void RestoreFromTray();
static void CheckAutoInject();
static void ShowColorPicker(const wchar_t* title, COLORREF* color, int colorId);
static void ResetTheme();
static void MonitorMinecraftProcess();
static bool ConfirmCloseMinecraft();

// Registry helpers
void SaveSetting(const wchar_t* key, int value) {
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\VoidInjector", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, key, 0, REG_DWORD, (BYTE*)&value, sizeof(value));
        RegCloseKey(hKey);
    }
}

int LoadSetting(const wchar_t* key, int defaultValue) {
    HKEY hKey;
    DWORD value = defaultValue;
    DWORD size = sizeof(value);
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\VoidInjector", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExW(hKey, key, 0, NULL, (BYTE*)&value, &size);
        RegCloseKey(hKey);
    }
    return value;
}

void SaveColorSetting(const wchar_t* key, COLORREF color) {
    SaveSetting(key, (int)color);
}

COLORREF LoadColorSetting(const wchar_t* key, COLORREF defaultColor) {
    return (COLORREF)LoadSetting(key, (int)defaultColor);
}

void SaveStringSetting(const wchar_t* key, const wchar_t* value) {
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\VoidInjector", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, key, 0, REG_SZ, (BYTE*)value, (wcslen(value) + 1) * sizeof(wchar_t));
        RegCloseKey(hKey);
    }
}

void LoadStringSetting(const wchar_t* key, wchar_t* value, int size, const wchar_t* defaultValue) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\VoidInjector", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD type = REG_SZ;
        DWORD dataSize = size * sizeof(wchar_t);
        if (RegQueryValueExW(hKey, key, 0, &type, (BYTE*)value, &dataSize) != ERROR_SUCCESS) {
            wcscpy_s(value, size, defaultValue);
        }
        RegCloseKey(hKey);
    } else {
        wcscpy_s(value, size, defaultValue);
    }
}

// Process helpers
DWORD FindProcess(const wchar_t* name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W entry = { sizeof(PROCESSENTRY32W) };
    if (Process32FirstW(snap, &entry)) {
        do {
            if (wcscmp(entry.szExeFile, name) == 0) {
                CloseHandle(snap);
                return entry.th32ProcessID;
            }
        } while (Process32NextW(snap, &entry));
    }
    CloseHandle(snap);
    return 0;
}

void KillProcess(DWORD pid) {
    HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProc) {
        TerminateProcess(hProc, 0);
        CloseHandle(hProc);
    }
}

DWORD WaitForProcess(const wchar_t* name, int timeoutSeconds = 60) {
    for (int i = 0; i < timeoutSeconds * 2; i++) {
        DWORD pid = FindProcess(name);
        if (pid) return pid;
        Sleep(500);
    }
    return 0;
}

std::wstring FindMinecraftExe() {
    std::vector<std::wstring> paths;

    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(L"C:\\Program Files\\WindowsApps\\*Minecraft.Windows.exe", &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            wchar_t fullPath[MAX_PATH];
            wsprintfW(fullPath, L"C:\\Program Files\\WindowsApps\\%s", findData.cFileName);
            if (GetFileAttributesW(fullPath) != INVALID_FILE_ATTRIBUTES) {
                paths.push_back(fullPath);
            }
        } while (FindNextFileW(hFind, &findData));
        FindClose(hFind);
    }

    const wchar_t* commonPaths[] = {
        L"C:\\Program Files\\Minecraft\\Minecraft.Windows.exe",
        L"C:\\Program Files (x86)\\Minecraft\\Minecraft.Windows.exe",
        L"D:\\Program Files\\Minecraft\\Minecraft.Windows.exe",
        L"C:\\Minecraft\\Minecraft.Windows.exe"
    };
    for (int i = 0; i < 4; i++) {
        if (GetFileAttributesW(commonPaths[i]) != INVALID_FILE_ATTRIBUTES) {
            paths.push_back(commonPaths[i]);
        }
    }

    for (const auto& path : paths) {
        if (GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES) {
            return path;
        }
    }
    return L"";
}

bool LaunchMinecraft() {
    SetStatus(AppState::Working, L"Searching for Minecraft installation...");
    std::wstring exePath = FindMinecraftExe();

    if (!exePath.empty()) {
        SetStatus(AppState::Working, L"Launching Minecraft from installation...");
        HINSTANCE result = ShellExecuteW(NULL, L"open", exePath.c_str(), NULL, NULL, SW_SHOW);
        if ((intptr_t)result > 32) return true;
    }

    SetStatus(AppState::Working, L"Attempting to start via App Store...");
    HINSTANCE result = ShellExecuteW(NULL, L"open", L"shell:AppsFolder\\Microsoft.MinecraftUWP_8wekyb3d8bbwe!App", NULL, NULL, SW_SHOW);
    if ((intptr_t)result > 32) return true;

    SetStatus(AppState::Working, L"Attempting to start via minecraft:// protocol...");
    result = ShellExecuteW(NULL, L"open", L"minecraft://", NULL, NULL, SW_SHOW);
    if ((intptr_t)result > 32) return true;

    return false;
}

bool InjectDLL(DWORD pid, const wchar_t* dllPath) {
    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProc) {
        SetStatus(AppState::Error, L"Failed to open process");
        return false;
    }

    size_t size = (wcslen(dllPath) + 1) * sizeof(wchar_t);
    LPVOID pMem = VirtualAllocEx(hProc, NULL, size, MEM_COMMIT, PAGE_READWRITE);
    if (!pMem) {
        CloseHandle(hProc);
        SetStatus(AppState::Error, L"VirtualAllocEx failed");
        return false;
    }

    WriteProcessMemory(hProc, pMem, dllPath, size, NULL);

    HMODULE k32 = GetModuleHandleW(L"kernel32.dll");
    LPTHREAD_START_ROUTINE pLoad = (LPTHREAD_START_ROUTINE)GetProcAddress(k32, "LoadLibraryW");

    HANDLE hThread = CreateRemoteThread(hProc, NULL, 0, pLoad, pMem, 0, NULL);
    if (!hThread) {
        VirtualFreeEx(hProc, pMem, 0, MEM_RELEASE);
        CloseHandle(hProc);
        SetStatus(AppState::Error, L"CreateRemoteThread failed");
        return false;
    }

    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    VirtualFreeEx(hProc, pMem, 0, MEM_RELEASE);
    CloseHandle(hProc);
    return true;
}

// Settings
static void ToggleAlwaysOnTop()
{
    g_alwaysOnTop = !g_alwaysOnTop;
    SaveSetting(L"AlwaysOnTop", g_alwaysOnTop ? 1 : 0);

    if (g_alwaysOnTop) {
        SetWindowPos(g_window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        SetStatus(AppState::Ready, L"Always on Top: Enabled");
    } else {
        SetWindowPos(g_window, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        SetStatus(AppState::Ready, L"Always on Top: Disabled");
    }
}

static void ToggleAutoInject()
{
    g_autoInject = !g_autoInject;
    SaveSetting(L"AutoInject", g_autoInject ? 1 : 0);
    SetStatus(AppState::Ready, g_autoInject ? L"Auto-Inject: Enabled" : L"Auto-Inject: Disabled");
}

static void SetInjectDelay(int seconds)
{
    g_injectDelay = seconds;
    SaveSetting(L"InjectDelay", seconds);
    wchar_t buf[64];
    wsprintfW(buf, L"Inject Delay: %d seconds", seconds);
    SetStatus(AppState::Ready, buf);
}

static void ToggleMinimizeToTray()
{
    g_minimizeToTray = !g_minimizeToTray;
    SaveSetting(L"MinimizeToTray", g_minimizeToTray ? 1 : 0);
    SetStatus(AppState::Ready, g_minimizeToTray ? L"Minimize to Tray: Enabled" : L"Minimize to Tray: Disabled");
}

static void ToggleStartWithWindows()
{
    g_startWithWindows = !g_startWithWindows;
    SaveSetting(L"StartWithWindows", g_startWithWindows ? 1 : 0);

    HKEY hKey;
    if (g_startWithWindows) {
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(NULL, path, MAX_PATH);
        if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
            RegSetValueExW(hKey, L"VoidInjector", 0, REG_SZ, (BYTE*)path, (wcslen(path) + 1) * sizeof(wchar_t));
            RegCloseKey(hKey);
        }
        SetStatus(AppState::Ready, L"Start with Windows: Enabled");
    } else {
        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
            RegDeleteValueW(hKey, L"VoidInjector");
            RegCloseKey(hKey);
        }
        SetStatus(AppState::Ready, L"Start with Windows: Disabled");
    }
}

static void ToggleStartMinimized()
{
    g_startMinimized = !g_startMinimized;
    SaveSetting(L"StartMinimized", g_startMinimized ? 1 : 0);
    SetStatus(AppState::Ready, g_startMinimized ? L"Start Minimized: Enabled" : L"Start Minimized: Disabled");
}

static void ToggleStartWithMinecraft()
{
    g_startWithMinecraft = !g_startWithMinecraft;
    SaveSetting(L"StartWithMinecraft", g_startWithMinecraft ? 1 : 0);
    g_monitorMinecraft = g_startWithMinecraft;

    if (g_startWithMinecraft) {
        SetStatus(AppState::Ready, L"Show on Minecraft Launch: Enabled - Monitoring...");
        DWORD pid = FindProcess(L"Minecraft.Windows.exe");
        if (pid) {
            SetStatus(AppState::Ready, L"Minecraft is already running!");
            if (g_startMinimized) {
                ShowWindow(g_window, SW_RESTORE);
            }
        }
    } else {
        SetStatus(AppState::Ready, L"Show on Minecraft Launch: Disabled");
    }
}

static void MonitorMinecraftProcess()
{
    if (!g_monitorMinecraft) return;

    static bool wasRunning = false;
    DWORD pid = FindProcess(L"Minecraft.Windows.exe");

    if (g_minecraftWasClosedByUser) {
        if (!pid) {
            g_minecraftWasClosedByUser = false;
            wasRunning = false;
        }
        return;
    }

    if (pid && !wasRunning) {
        wasRunning = true;
        SetStatus(AppState::Ready, L"Minecraft detected! Injector ready.");

        if (IsWindowVisible(g_window) == FALSE) {
            ShowWindow(g_window, SW_SHOW);
        }
        if (IsIconic(g_window)) {
            ShowWindow(g_window, SW_RESTORE);
        }
        SetForegroundWindow(g_window);
        FlashWindow(g_window, TRUE);

        if (g_autoInject && g_dllPath[0] != L'\0') {
            SetStatus(AppState::Working, L"Auto-Inject: Minecraft detected!");
            Sleep(3000);

            if (InjectDLL(pid, g_dllPath)) {
                SetStatus(AppState::Success, L"Auto-Inject: DLL injected successfully!");
            } else {
                SetStatus(AppState::Error, L"Auto-Inject: Injection failed");
            }
        }
    } else if (!pid && wasRunning) {
        wasRunning = false;
        if (g_monitorMinecraft) {
            SetStatus(AppState::Ready, L"Monitoring for Minecraft...");
        }
    }
}

// Tray
static void MinimizeToTray()
{
    if (!g_minimizeToTray) return;

    HICON hTrayIcon = LoadIconW(GetModuleHandleW(NULL), MAKEINTRESOURCE(IDI_MAIN_ICON));
    if (!hTrayIcon) {
        hTrayIcon = LoadIconW(GetModuleHandleW(NULL), IDI_APPLICATION);
    }

    g_trayData.cbSize = sizeof(NOTIFYICONDATAW);
    g_trayData.hWnd = g_window;
    g_trayData.uID = 1;
    g_trayData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_trayData.uCallbackMessage = WM_USER + 1;
    g_trayData.hIcon = hTrayIcon;
    wcscpy_s(g_trayData.szTip, L"Void Injector");

    Shell_NotifyIconW(NIM_ADD, &g_trayData);
    ShowWindow(g_window, SW_HIDE);
    g_isTray = true;
}

static void RestoreFromTray()
{
    Shell_NotifyIconW(NIM_DELETE, &g_trayData);
    ShowWindow(g_window, SW_SHOW);
    g_isTray = false;
}

static void CheckAutoInject()
{
    if (!g_autoInject || g_dllPath[0] == L'\0' || g_isInjecting) return;

    if (g_minecraftWasClosedByUser) return;

    DWORD pid = FindProcess(L"Minecraft.Windows.exe");
    if (pid && !g_wasRunning) {
        g_wasRunning = true;
        SetStatus(AppState::Working, L"Auto-Inject: Minecraft detected!");
        Sleep(5000);

        if (InjectDLL(pid, g_dllPath)) {
            SetStatus(AppState::Success, L"Auto-Inject: DLL injected successfully!");
        } else {
            SetStatus(AppState::Error, L"Auto-Inject: Injection failed");
        }
    }
}

// Theme
static void ApplyTheme()
{
    DeleteObject(g_bgBrush);
    g_bgBrush = CreateSolidBrush(BG);
    if (g_window) {
        SetClassLongPtr(g_window, GCLP_HBRBACKGROUND, (LONG_PTR)g_bgBrush);
        InvalidateRect(g_window, nullptr, TRUE);
    }
}

static void ResetTheme()
{
    BG = DEFAULT_BG;
    CARD = DEFAULT_CARD;
    CARD2 = DEFAULT_CARD2;
    BORDER = DEFAULT_BORDER;
    TEXT = DEFAULT_TEXT;
    MUTED = DEFAULT_MUTED;
    GREEN = DEFAULT_GREEN;
    CYAN = DEFAULT_CYAN;
    RED = DEFAULT_RED;
    YELLOW = DEFAULT_YELLOW;
    ORANGE = DEFAULT_ORANGE;
    HOVER = DEFAULT_HOVER;
    g_isCustomTheme = false;
    SaveSetting(L"IsCustomTheme", 0);
    ApplyTheme();
    SetStatus(AppState::Ready, L"Theme reset to default");
}

static void ShowColorPicker(const wchar_t* title, COLORREF* color, int colorId)
{
    CHOOSECOLORW cc = {};
    static COLORREF customColors[16] = {0};
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = g_window;
    cc.lpCustColors = customColors;
    cc.rgbResult = *color;
    cc.Flags = CC_RGBINIT | CC_FULLOPEN;

    if (ChooseColorW(&cc)) {
        *color = cc.rgbResult;
        g_isCustomTheme = true;
        SaveSetting(L"IsCustomTheme", 1);
        SaveColorSetting(L"ColorBG", BG);
        SaveColorSetting(L"ColorCARD", CARD);
        SaveColorSetting(L"ColorCARD2", CARD2);
        SaveColorSetting(L"ColorBORDER", BORDER);
        SaveColorSetting(L"ColorTEXT", TEXT);
        SaveColorSetting(L"ColorMUTED", MUTED);
        SaveColorSetting(L"ColorGREEN", GREEN);
        SaveColorSetting(L"ColorCYAN", CYAN);
        SaveColorSetting(L"ColorRED", RED);
        SaveColorSetting(L"ColorYELLOW", YELLOW);
        SaveColorSetting(L"ColorORANGE", ORANGE);
        SaveColorSetting(L"ColorHOVER", HOVER);
        ApplyTheme();
        SetStatus(AppState::Ready, L"Theme updated");
    }
}

// Fonts
static HFONT MakeFont(int height, int weight)
{
    return CreateFontW(
        height, 0, 0, 0, weight,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI"
    );
}

// Drawing helpers
static void FillRound(HDC dc, const RECT& r, COLORREF color, int radius = 12)
{
    HBRUSH brush = CreateSolidBrush(color);
    HGDIOBJ oldBrush = SelectObject(dc, brush);

    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HGDIOBJ oldPen = SelectObject(dc, pen);

    RoundRect(dc, r.left, r.top, r.right, r.bottom, radius, radius);

    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);

    DeleteObject(pen);
    DeleteObject(brush);
}

static void DrawTextAt(
    HDC dc,
    const wchar_t* text,
    const RECT& r,
    HFONT font,
    COLORREF color,
    UINT flags = DT_LEFT)
{
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);

    HGDIOBJ oldFont = SelectObject(dc, font);

    DrawTextW(
        dc,
        text,
        -1,
        const_cast<RECT*>(&r),
        flags | DT_VCENTER | DT_SINGLELINE
    );

    SelectObject(dc, oldFont);
}

static void DrawCentered(
    HDC dc,
    const wchar_t* text,
    const RECT& r,
    HFONT font,
    COLORREF color)
{
    DrawTextAt(dc, text, r, font, color, DT_CENTER);
}

static bool InRect(const RECT& r, int x, int y)
{
    return x >= r.left && x < r.right &&
           y >= r.top && y < r.bottom;
}

static COLORREF StatusColor()
{
    switch (g_state)
    {
    case AppState::Working: return YELLOW;
    case AppState::Success: return GREEN;
    case AppState::Error:   return RED;
    case AppState::Ready:   return CYAN;
    default:                return MUTED;
    }
}

// Status
static void SetStatus(AppState state, const wchar_t* text)
{
    g_state = state;

    lstrcpynW(
        g_statusText,
        text,
        static_cast<int>(sizeof(g_statusText) / sizeof(g_statusText[0]))
    );

    InvalidateRect(g_window, nullptr, FALSE);
}

static void UpdateStatus()
{
    if (g_isInjecting) return;

    DWORD pid = FindProcess(L"Minecraft.Windows.exe");

    if (pid) {
        g_wasRunning = true;
        wchar_t buf[64];
        wsprintfW(buf, L"Minecraft running (PID: %d)", pid);
        SetStatus(AppState::Ready, buf);
    } else {
        if (g_wasRunning) {
            g_wasRunning = false;
            SetStatus(AppState::Ready, L"Minecraft has been stopped");
        } else {
            if (g_startWithMinecraft && g_dllPath[0] != L'\0') {
                SetStatus(AppState::Ready, L"Monitoring for Minecraft...");
            } else if (g_dllPath[0] != L'\0') {
                SetStatus(AppState::Ready, L"Ready - Select DLL and click Inject");
            } else {
                SetStatus(AppState::Ready, L"Ready - Select a DLL to inject");
            }
        }
    }
}

// Confirmation
static bool ConfirmCloseMinecraft()
{
    DWORD pid = FindProcess(L"Minecraft.Windows.exe");
    if (!pid) {
        SetStatus(AppState::Error, L"Minecraft is not running");
        return false;
    }

    SetForegroundWindow(g_window);
    SetActiveWindow(g_window);
    BringWindowToTop(g_window);
    FlashWindow(g_window, TRUE);

    int result = MessageBoxW(g_window,
        L"WARNING: CLOSE MINECRAFT?\n\nThis will terminate the game immediately.\nAny unsaved progress will be lost!\n\nAre you sure you want to continue?",
        L"Confirm Close",
        MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2 | MB_TOPMOST | MB_SETFOREGROUND);

    return (result == IDYES);
}

// UI actions
static void BrowseForDLL()
{
    OPENFILENAMEW dialog{};
    wchar_t file[MAX_PATH] = L"";

    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = g_window;
    dialog.lpstrFilter =
        L"DLL Files (*.dll)\0*.dll\0"
        L"All Files (*.*)\0*.*\0";
    dialog.lpstrFile = file;
    dialog.nMaxFile = MAX_PATH;
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (GetOpenFileNameW(&dialog))
    {
        lstrcpynW(g_dllPath, file, MAX_PATH);
        SaveStringSetting(L"DLLPath", g_dllPath);
        SetStatus(AppState::Ready, L"DLL selected - Ready to inject");
    }
}

static void OnInject()
{
    if (g_dllPath[0] == L'\0')
    {
        SetStatus(AppState::Error, L"Please select a DLL file first");
        return;
    }

    if (g_isInjecting) {
        SetStatus(AppState::Error, L"Injection already in progress");
        return;
    }

    g_isInjecting = true;
    SetStatus(AppState::Working, L"Starting injection process...");

    g_pid = FindProcess(L"Minecraft.Windows.exe");
    if (!g_pid) {
        SetStatus(AppState::Working, L"Minecraft not running - Launching now...");
        if (!LaunchMinecraft()) {
            SetStatus(AppState::Error, L"Failed to launch Minecraft");
            g_isInjecting = false;
            return;
        }
        SetStatus(AppState::Working, L"Waiting for Minecraft to start up...");
        g_pid = WaitForProcess(L"Minecraft.Windows.exe", 60);
        if (!g_pid) {
            SetStatus(AppState::Error, L"Timed out waiting for Minecraft");
            g_isInjecting = false;
            return;
        }
        SetStatus(AppState::Working, L"Minecraft started successfully!");
        g_minecraftWasClosedByUser = false;
    } else {
        SetStatus(AppState::Working, L"Minecraft is running - Preparing to inject...");
    }

    wchar_t delayMsg[64];
    wsprintfW(delayMsg, L"Waiting %d seconds for main menu...", g_injectDelay);
    SetStatus(AppState::Working, delayMsg);
    Sleep(g_injectDelay * 1000);

    SetStatus(AppState::Working, L"Injecting DLL into Minecraft process...");
    if (InjectDLL(g_pid, g_dllPath)) {
        SetStatus(AppState::Success, L"DLL injection completed successfully!");
        g_wasRunning = true;
    } else {
        SetStatus(AppState::Error, L"DLL injection failed - Check if Minecraft is running properly");
    }

    g_isInjecting = false;
    UpdateStatus();
}

static void OnCloseMinecraft()
{
    if (g_isInjecting) {
        SetStatus(AppState::Error, L"Cannot close Minecraft during injection");
        return;
    }

    if (!ConfirmCloseMinecraft()) {
        SetStatus(AppState::Ready, L"Close cancelled");
        return;
    }

    DWORD pid = FindProcess(L"Minecraft.Windows.exe");
    if (pid) {
        g_minecraftWasClosedByUser = true;
        g_wasRunning = false;
        SetStatus(AppState::Working, L"Closing Minecraft process...");
        KillProcess(pid);
        SetStatus(AppState::Ready, L"Minecraft has been closed");
        UpdateStatus();
    } else {
        SetStatus(AppState::Error, L"Minecraft is not running");
    }
}

static void ClearDLL()
{
    if (g_isInjecting) {
        SetStatus(AppState::Error, L"Cannot clear during injection");
        return;
    }

    g_dllPath[0] = L'\0';
    SetStatus(AppState::Ready, L"Cleared - Select a DLL to inject");
}

// UI drawing helpers
static void FillGradient(HDC dc, const RECT& r, COLORREF top, COLORREF bottom)
{
    int h = r.bottom - r.top;
    if (h <= 0) return;

    for (int y = 0; y < h; ++y) {
        double t = (double)y / (double)(h - 1);
        int r1 = GetRValue(top), g1 = GetGValue(top), b1 = GetBValue(top);
        int r2 = GetRValue(bottom), g2 = GetGValue(bottom), b2 = GetBValue(bottom);
        COLORREF c = RGB(
            (int)(r1 + (r2 - r1) * t),
            (int)(g1 + (g2 - g1) * t),
            (int)(b1 + (b2 - b1) * t)
        );
        HPEN pen = CreatePen(PS_SOLID, 1, c);
        HGDIOBJ old = SelectObject(dc, pen);
        MoveToEx(dc, r.left, r.top + y, nullptr);
        LineTo(dc, r.right, r.top + y);
        SelectObject(dc, old);
        DeleteObject(pen);
    }
}

static void DrawPanel(HDC dc, const RECT& r, COLORREF fill, COLORREF border, int radius = 12)
{
    FillRound(dc, r, fill, radius);
    HPEN pen = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ old = SelectObject(dc, pen);
    HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
    RoundRect(dc, r.left, r.top, r.right, r.bottom, radius, radius);
    SelectObject(dc, oldBrush);
    SelectObject(dc, old);
    DeleteObject(pen);
}

static void DrawBlueGlow(HDC dc, const RECT& r)
{
    RECT glow = {r.left - 2, r.top - 2, r.right + 2, r.bottom + 2};
    FillRound(dc, glow, RGB(7, 42, 92), 13);
}

static void DrawStatusDot(HDC dc, int x, int y, COLORREF color)
{
    HBRUSH brush = CreateSolidBrush(color);
    HGDIOBJ old = SelectObject(dc, brush);
    Ellipse(dc, x, y, x + 8, y + 8);
    SelectObject(dc, old);
    DeleteObject(brush);
}

// Paint
static void PaintUI(HDC dc)
{
    RECT client{};
    GetClientRect(g_window, &client);
    FillRect(dc, &client, g_bgBrush);

    // Header background.
    RECT topBg{0, 0, 620, 94};
    FillGradient(dc, topBg, RGB(5, 13, 29), RGB(2, 5, 13));

    // Header.
    RECT logoAccent{25, 20, 30, 69};
    FillRound(dc, logoAccent, CYAN, 3);

    RECT title{43, 15, 330, 48};
    DrawTextAt(dc, L"VOID", title, g_titleFont, TEXT);

    RECT subtitle{45, 48, 300, 72};
    DrawTextAt(dc, L"DLL INJECTOR", subtitle, g_smallFont, CYAN);

    RECT version{425, 20, 500, 43};
    DrawTextAt(dc, L"v1.0.0", version, g_smallFont, MUTED, DT_RIGHT);

    RECT settingsBtn{510, 17, 590, 49};
    DrawPanel(dc, settingsBtn,
              (g_hover == 5) ? RGB(18, 66, 135) : CARD2,
              (g_hover == 5) ? CYAN : BORDER, 8);
    RECT settingsText{510, 17, 590, 49};
    DrawCentered(dc, L"SETTINGS", settingsText, g_smallFont,
                 (g_hover == 5) ? TEXT : MUTED);

    // Connection status.
    RECT connected{458, 57, 590, 82};
    DrawPanel(dc, connected, RGB(6, 18, 37), RGB(17, 65, 125), 13);
    DrawStatusDot(dc, 470, 65, CYAN);
    RECT connectedText{485, 57, 580, 82};
    DrawCentered(dc, L"SYSTEM ONLINE", connectedText, g_smallFont, CYAN);

    // DLL selection.
    RECT dllCard{25, 104, 595, 210};
    DrawPanel(dc, dllCard, CARD, BORDER, 14);

    RECT dllLabel{45, 117, 160, 140};
    DrawTextAt(dc, L"SELECTED DLL", dllLabel, g_smallFont, MUTED);

    // Selected file.
    RECT fileBoxOuter{44, 146, 576, 196};
    DrawPanel(dc, fileBoxOuter, RGB(5, 12, 25), RGB(18, 62, 120), 10);

    RECT iconBox{54, 153, 84, 189};
    FillRound(dc, iconBox, RGB(9, 44, 91), 8);
    RECT iconText{54, 153, 84, 189};
    DrawCentered(dc, L"D", iconText, g_bodyFont, CYAN);

    RECT pathText{96, 153, 432, 189};
    if (g_dllPath[0])
        // Keep long paths inside the file field instead of drawing over the
        // SELECT button. DT_END_ELLIPSIS adds "..." when the path is too long.
        DrawTextAt(dc, g_dllPath, pathText, g_bodyFont, TEXT,
                   DT_LEFT | DT_END_ELLIPSIS);
    else
        DrawTextAt(dc, L"No DLL selected", pathText, g_bodyFont, MUTED);

    RECT browse{442, 153, 566, 189};
    DrawPanel(dc, browse,
              (g_hover == 1) ? RGB(0, 155, 255) : RGB(0, 113, 215),
              (g_hover == 1) ? RGB(110, 205, 255) : RGB(0, 145, 255), 8);
    RECT browseText{442, 153, 566, 189};
    DrawCentered(dc, L"SELECT", browseText, g_bodyFont, RGB(245, 249, 255));

    // Minecraft status.
    RECT minecraftCard{25, 224, 595, 310};
    DrawPanel(dc, minecraftCard, CARD, BORDER, 14);

    RECT minecraftLabel{45, 237, 180, 258};
    DrawTextAt(dc, L"MINECRAFT", minecraftLabel, g_smallFont, MUTED);

    DWORD pid = FindProcess(L"Minecraft.Windows.exe");
    COLORREF mcStatusColor;
    const wchar_t* mcStatusText;

    if (pid) {
        mcStatusColor = GREEN;
        mcStatusText = L"RUNNING";
    } else if (g_wasRunning) {
        mcStatusColor = RED;
        mcStatusText = L"STOPPED";
    } else {
        mcStatusColor = ORANGE;
        mcStatusText = L"NOT RUNNING";
    }

    RECT running{45, 266, 180, 294};
    DrawPanel(dc, running, RGB(6, 17, 32), RGB(20, 48, 80), 14);
    DrawStatusDot(dc, 58, 276, mcStatusColor);
    RECT runningText{74, 266, 170, 294};
    DrawTextAt(dc, mcStatusText, runningText, g_smallFont, mcStatusColor);

    RECT processLabel{215, 237, 320, 258};
    DrawTextAt(dc, L"PROCESS", processLabel, g_smallFont, MUTED);

    RECT processValue{215, 266, 425, 294};
    DrawTextAt(dc, L"Minecraft.Windows.exe", processValue, g_bodyFont, TEXT);

    RECT pidLabel{455, 237, 510, 258};
    DrawTextAt(dc, L"PID", pidLabel, g_smallFont, MUTED);

    RECT pidValue{455, 266, 535, 294};
    wchar_t pidStr[16];
    if (pid) wsprintfW(pidStr, L"%d", pid);
    else wcscpy_s(pidStr, L"--");
    DrawTextAt(dc, pidStr, pidValue, g_bodyFont, TEXT);

    if (g_autoInject) {
        RECT autoLabel{510, 237, 575, 258};
        DrawTextAt(dc, L"AUTO", autoLabel, g_smallFont, CYAN, DT_RIGHT);
    }

    // Main action button.
    RECT injectGlow{23, 324, 597, 379};
    DrawBlueGlow(dc, injectGlow);

    RECT inject{25, 326, 595, 377};
    FillGradient(dc, inject,
                 (g_hover == 2) ? RGB(255, 255, 255) : RGB(238, 241, 246),
                 (g_hover == 2) ? RGB(218, 229, 241) : RGB(202, 208, 218));

    RECT injectText{25, 326, 595, 377};
    DrawCentered(dc, g_isInjecting ? L"INJECTING..." : L"INJECT DLL",
                 injectText, g_bigFont, RGB(4, 11, 23));

    // Bottom buttons.
    RECT closeButton{25, 395, 298, 437};
    DrawPanel(dc, closeButton,
              (g_hover == 3) ? RGB(30, 35, 44) : CARD2,
              (g_hover == 3) ? RGB(75, 90, 110) : BORDER, 10);
    RECT closeText{25, 395, 298, 437};
    DrawCentered(dc, L"CLOSE MINECRAFT", closeText, g_bodyFont, TEXT);

    RECT clearButton{322, 395, 595, 437};
    DrawPanel(dc, clearButton,
              (g_hover == 4) ? RGB(30, 35, 44) : CARD2,
              (g_hover == 4) ? RGB(75, 90, 110) : BORDER, 10);
    RECT clearText{322, 395, 595, 437};
    DrawCentered(dc, L"CLEAR DLL", clearText, g_bodyFont, MUTED);

    // Status bar.
    HPEN linePen = CreatePen(PS_SOLID, 1, RGB(18, 48, 88));
    HGDIOBJ oldPen = SelectObject(dc, linePen);
    MoveToEx(dc, 25, 454, nullptr);
    LineTo(dc, 595, 454);
    SelectObject(dc, oldPen);
    DeleteObject(linePen);

    DrawStatusDot(dc, 29, 467, StatusColor());
    RECT footerText{46, 458, 320, 485};
    DrawTextAt(dc, g_statusText, footerText, g_smallFont, MUTED,
               DT_LEFT | DT_END_ELLIPSIS);

    RECT creditText{325, 458, 590, 485};
    DrawTextAt(dc, L"Made by Ongbay", creditText, g_smallFont, MUTED, DT_RIGHT);

    if (g_alwaysOnTop) {
        RECT topText{540, 458, 590, 485};
        DrawTextAt(dc, L"TOP", topText, g_smallFont, CYAN, DT_RIGHT);
    }
}

// Window procedure
static LRESULT CALLBACK WindowProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (message)
    {
    case WM_ERASEBKGND:
        return 1;

    case WM_TIMER:
        if (wParam == 1) {
            UpdateStatus();
            CheckAutoInject();
            MonitorMinecraftProcess();
        }
        return 0;

    case WM_USER + 1:
        if (lParam == WM_LBUTTONDOWN) {
            RestoreFromTray();
        }
        return 0;

    case WM_SYSCOMMAND:
        if (wParam == SC_MINIMIZE && g_minimizeToTray) {
            MinimizeToTray();
            return 0;
        }
        break;

    case WM_MOUSEMOVE:
    {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);

        RECT browse{442, 153, 566, 189};
        RECT inject{25, 326, 595, 377};
        RECT closeButton{25, 395, 298, 437};
        RECT clearButton{322, 395, 595, 437};
        RECT settingsBtn{510, 17, 590, 49};

        int newHover = 0;

        if (InRect(settingsBtn, x, y)) newHover = 5;
        else if (InRect(browse, x, y)) newHover = 1;
        else if (InRect(inject, x, y)) newHover = 2;
        else if (InRect(closeButton, x, y)) newHover = 3;
        else if (InRect(clearButton, x, y)) newHover = 4;

        if (newHover != g_hover)
        {
            g_hover = newHover;
            InvalidateRect(hwnd, nullptr, FALSE);
        }

        if (!g_mouseTracking)
        {
            TRACKMOUSEEVENT tme{};
            tme.cbSize = sizeof(tme);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hwnd;

            if (TrackMouseEvent(&tme))
                g_mouseTracking = true;
        }

        return 0;
    }

    case WM_MOUSELEAVE:
        g_mouseTracking = false;

        if (g_hover != 0)
        {
            g_hover = 0;
            InvalidateRect(hwnd, nullptr, FALSE);
        }

        return 0;

    case WM_LBUTTONDOWN:
    {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);

        RECT browse{442, 153, 566, 189};
        RECT inject{25, 326, 595, 377};
        RECT closeButton{25, 395, 298, 437};
        RECT clearButton{322, 395, 595, 437};
        RECT settingsBtn{510, 17, 590, 49};

        if (InRect(settingsBtn, x, y)) {
            HMENU hMenu = CreatePopupMenu();

            AppendMenuW(hMenu, MF_STRING | (g_alwaysOnTop ? MF_CHECKED : MF_UNCHECKED), 1001, L"Always on Top");
            AppendMenuW(hMenu, MF_STRING | (g_autoInject ? MF_CHECKED : MF_UNCHECKED), 1002, L"Auto-Inject on Launch");
            AppendMenuW(hMenu, MF_STRING | (g_startWithMinecraft ? MF_CHECKED : MF_UNCHECKED), 1007, L"Show on Minecraft Launch");

            HMENU delayMenu = CreatePopupMenu();
            AppendMenuW(delayMenu, MF_STRING | (g_injectDelay == 3 ? MF_CHECKED : MF_UNCHECKED), 1010, L"3 seconds");
            AppendMenuW(delayMenu, MF_STRING | (g_injectDelay == 5 ? MF_CHECKED : MF_UNCHECKED), 1011, L"5 seconds");
            AppendMenuW(delayMenu, MF_STRING | (g_injectDelay == 8 ? MF_CHECKED : MF_UNCHECKED), 1012, L"8 seconds");
            AppendMenuW(delayMenu, MF_STRING | (g_injectDelay == 10 ? MF_CHECKED : MF_UNCHECKED), 1013, L"10 seconds");
            AppendMenuW(delayMenu, MF_STRING | (g_injectDelay == 15 ? MF_CHECKED : MF_UNCHECKED), 1014, L"15 seconds");
            AppendMenuW(delayMenu, MF_STRING | (g_injectDelay == 20 ? MF_CHECKED : MF_UNCHECKED), 1015, L"20 seconds");
            AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)delayMenu, L"Inject Delay");

            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

            AppendMenuW(hMenu, MF_STRING | (g_minimizeToTray ? MF_CHECKED : MF_UNCHECKED), 1003, L"Minimize to Tray");
            AppendMenuW(hMenu, MF_STRING | (g_startWithWindows ? MF_CHECKED : MF_UNCHECKED), 1005, L"Start with Windows");
            AppendMenuW(hMenu, MF_STRING | (g_startMinimized ? MF_CHECKED : MF_UNCHECKED), 1006, L"Start Minimized");

            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

            HMENU themeMenu = CreatePopupMenu();
            AppendMenuW(themeMenu, MF_STRING, 1020, L"Customize Colors...");
            AppendMenuW(themeMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(themeMenu, MF_STRING, 1021, L"Background");
            AppendMenuW(themeMenu, MF_STRING, 1022, L"Card Color");
            AppendMenuW(themeMenu, MF_STRING, 1023, L"Card2 Color");
            AppendMenuW(themeMenu, MF_STRING, 1024, L"Border Color");
            AppendMenuW(themeMenu, MF_STRING, 1025, L"Text Color");
            AppendMenuW(themeMenu, MF_STRING, 1026, L"Muted Text");
            AppendMenuW(themeMenu, MF_STRING, 1027, L"Green Accent");
            AppendMenuW(themeMenu, MF_STRING, 1028, L"Cyan Accent");
            AppendMenuW(themeMenu, MF_STRING, 1029, L"Red Accent");
            AppendMenuW(themeMenu, MF_STRING, 1030, L"Yellow Accent");
            AppendMenuW(themeMenu, MF_STRING, 1031, L"Orange Accent");
            AppendMenuW(themeMenu, MF_STRING, 1032, L"Hover Color");
            AppendMenuW(themeMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(themeMenu, MF_STRING, 1033, L"Reset Theme");
            AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)themeMenu, L"Theme");

            POINT pt;
            GetCursorPos(&pt);
            int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, NULL);
            DestroyMenu(hMenu);

            switch (cmd) {
                case 1001: ToggleAlwaysOnTop(); break;
                case 1002: ToggleAutoInject(); break;
                case 1003: ToggleMinimizeToTray(); break;
                case 1005: ToggleStartWithWindows(); break;
                case 1006: ToggleStartMinimized(); break;
                case 1007: ToggleStartWithMinecraft(); break;
                case 1010: SetInjectDelay(3); break;
                case 1011: SetInjectDelay(5); break;
                case 1012: SetInjectDelay(8); break;
                case 1013: SetInjectDelay(10); break;
                case 1014: SetInjectDelay(15); break;
                case 1015: SetInjectDelay(20); break;
                case 1020: ShowColorPicker(L"Customize Theme", &BG, 0); break;
                case 1021: ShowColorPicker(L"Background Color", &BG, 1); break;
                case 1022: ShowColorPicker(L"Card Color", &CARD, 2); break;
                case 1023: ShowColorPicker(L"Card2 Color", &CARD2, 3); break;
                case 1024: ShowColorPicker(L"Border Color", &BORDER, 4); break;
                case 1025: ShowColorPicker(L"Text Color", &TEXT, 5); break;
                case 1026: ShowColorPicker(L"Muted Text Color", &MUTED, 6); break;
                case 1027: ShowColorPicker(L"Green Accent", &GREEN, 7); break;
                case 1028: ShowColorPicker(L"Cyan Accent", &CYAN, 8); break;
                case 1029: ShowColorPicker(L"Red Accent", &RED, 9); break;
                case 1030: ShowColorPicker(L"Yellow Accent", &YELLOW, 10); break;
                case 1031: ShowColorPicker(L"Orange Accent", &ORANGE, 11); break;
                case 1032: ShowColorPicker(L"Hover Color", &HOVER, 12); break;
                case 1033: ResetTheme(); break;
            }
        }
        else if (InRect(browse, x, y))
            BrowseForDLL();
        else if (InRect(inject, x, y))
            OnInject();
        else if (InRect(closeButton, x, y))
            OnCloseMinecraft();
        else if (InRect(clearButton, x, y))
            ClearDLL();

        return 0;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps{};
        HDC windowDC = BeginPaint(hwnd, &ps);

        RECT client{};
        GetClientRect(hwnd, &client);

        const int width = client.right - client.left;
        const int height = client.bottom - client.top;

        HDC bufferDC = CreateCompatibleDC(windowDC);
        HBITMAP bufferBitmap = CreateCompatibleBitmap(windowDC, width, height);
        HGDIOBJ oldBitmap = SelectObject(bufferDC, bufferBitmap);

        FillRect(bufferDC, &client, g_bgBrush);
        PaintUI(bufferDC);

        BitBlt(
            windowDC,
            0, 0,
            width, height,
            bufferDC,
            0, 0,
            SRCCOPY
        );

        SelectObject(bufferDC, oldBitmap);
        DeleteObject(bufferBitmap);
        DeleteDC(bufferDC);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        if (g_isTray) {
            Shell_NotifyIconW(NIM_DELETE, &g_trayData);
        }
        KillTimer(hwnd, 1);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

// Entry point
int WINAPI WinMain(
    HINSTANCE instance,
    HINSTANCE,
    LPSTR,
    int showCommand)
{
    // Load saved settings.
    g_alwaysOnTop = LoadSetting(L"AlwaysOnTop", 0) == 1;
    g_autoInject = LoadSetting(L"AutoInject", 0) == 1;
    g_injectDelay = LoadSetting(L"InjectDelay", 8);
    g_minimizeToTray = LoadSetting(L"MinimizeToTray", 0) == 1;
    g_startWithWindows = LoadSetting(L"StartWithWindows", 0) == 1;
    g_startMinimized = LoadSetting(L"StartMinimized", 0) == 1;
    g_startWithMinecraft = LoadSetting(L"StartWithMinecraft", 0) == 1;
    g_isCustomTheme = LoadSetting(L"IsCustomTheme", 0) == 1;
    g_monitorMinecraft = g_startWithMinecraft;

    LoadStringSetting(L"DLLPath", g_dllPath, MAX_PATH, L"");

    // Restore custom colors.
    if (g_isCustomTheme) {
        BG = LoadColorSetting(L"ColorBG", DEFAULT_BG);
        CARD = LoadColorSetting(L"ColorCARD", DEFAULT_CARD);
        CARD2 = LoadColorSetting(L"ColorCARD2", DEFAULT_CARD2);
        BORDER = LoadColorSetting(L"ColorBORDER", DEFAULT_BORDER);
        TEXT = LoadColorSetting(L"ColorTEXT", DEFAULT_TEXT);
        MUTED = LoadColorSetting(L"ColorMUTED", DEFAULT_MUTED);
        GREEN = LoadColorSetting(L"ColorGREEN", DEFAULT_GREEN);
        CYAN = LoadColorSetting(L"ColorCYAN", DEFAULT_CYAN);
        RED = LoadColorSetting(L"ColorRED", DEFAULT_RED);
        YELLOW = LoadColorSetting(L"ColorYELLOW", DEFAULT_YELLOW);
        ORANGE = LoadColorSetting(L"ColorORANGE", DEFAULT_ORANGE);
        HOVER = LoadColorSetting(L"ColorHOVER", DEFAULT_HOVER);
    }

    g_bgBrush = CreateSolidBrush(BG);
    g_titleFont = MakeFont(-27, FW_BOLD);
    g_bigFont   = MakeFont(-18, FW_BOLD);
    g_bodyFont  = MakeFont(-14, FW_NORMAL);
    g_smallFont = MakeFont(-11, FW_SEMIBOLD);

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.hInstance = instance;
    wc.lpfnWndProc = WindowProc;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = g_bgBrush;
    wc.lpszClassName = L"VoidInjector";
    wc.style = CS_HREDRAW | CS_VREDRAW;

    wc.hIcon = LoadIconW(instance, MAKEINTRESOURCE(IDI_MAIN_ICON));
    if (!wc.hIcon) {
        wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    }

    if (!RegisterClassExW(&wc))
        return 1;

    // PaintUI is based on a 620x510 client area.
    const DWORD windowStyle =
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT windowRect{0, 0, 620, 495};
    AdjustWindowRectEx(&windowRect, windowStyle, FALSE, 0);

    g_window = CreateWindowExW(
        0,
        wc.lpszClassName,
        L"Void DLL Injector",
        windowStyle,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        instance,
        nullptr
    );

    if (!g_window)
        return 1;

    HICON hIcon = LoadIconW(instance, MAKEINTRESOURCE(IDI_MAIN_ICON));
    if (hIcon) {
        SendMessage(g_window, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        SendMessage(g_window, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    }

    if (g_alwaysOnTop) {
        SetWindowPos(g_window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }

    SetTimer(g_window, 1, 2000, nullptr);

    if (g_startMinimized) {
        ShowWindow(g_window, SW_MINIMIZE);
    } else {
        ShowWindow(g_window, showCommand);
    }
    UpdateWindow(g_window);

    if (g_startWithMinecraft) {
        DWORD pid = FindProcess(L"Minecraft.Windows.exe");
        if (pid) {
            SetStatus(AppState::Ready, L"Minecraft is already running!");
            if (g_startMinimized) {
                ShowWindow(g_window, SW_RESTORE);
            }
        } else {
            SetStatus(AppState::Ready, L"Monitoring for Minecraft...");
        }
    }

    MSG msg{};

    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    DeleteObject(g_titleFont);
    DeleteObject(g_bigFont);
    DeleteObject(g_bodyFont);
    DeleteObject(g_smallFont);
    DeleteObject(g_bgBrush);

    return static_cast<int>(msg.wParam);
}
