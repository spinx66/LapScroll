#include <windows.h>
#include <shellapi.h>
#include <string>
#include <cmath>
#include "LapScrollShared.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")

// --------------------------------------------------
// IDs
// --------------------------------------------------
constexpr int ID_TIMER   = 1;
constexpr int ID_HOTKEY  = 2;
constexpr int ID_TRAYICON = 3;

constexpr int IDM_OPEN_SETTINGS = 1001;
constexpr int IDM_TOGGLE_PAUSE  = 1002;
constexpr int IDM_EXIT_APP      = 1003;
constexpr int IDM_RELOAD        = 1004;

// --------------------------------------------------
// Globals
// --------------------------------------------------
HINSTANCE g_hInst = nullptr;
HWND g_hwndHidden = nullptr;

LapScrollConfig g_cfg{};
bool g_scrollMode = false;
bool g_paused = false;
bool g_trayAdded = false;

POINT g_lockPos{};
POINT g_lastPos{};

const char* kMutexName = "LapScrollBackgroundSingleton";

// --------------------------------------------------
// Helpers
// --------------------------------------------------
void SetTrayTip(const char* tip) {
    NOTIFYICONDATAA nid{};
    nid.cbSize = sizeof(nid);
    nid.hWnd = g_hwndHidden;
    nid.uID = ID_TRAYICON;
    nid.uFlags = NIF_TIP;
    strncpy_s(nid.szTip, tip, _TRUNCATE);
    Shell_NotifyIconA(NIM_MODIFY, &nid);
}

void AddTrayIcon() {
    if (g_trayAdded) return;

    NOTIFYICONDATAA nid{};
    nid.cbSize = sizeof(nid);
    nid.hWnd = g_hwndHidden;
    nid.uID = ID_TRAYICON;
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid.uCallbackMessage = WM_LAPSCROLL_TRAY;
    nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    strncpy_s(nid.szTip, "LapScroll", _TRUNCATE);

    if (Shell_NotifyIconA(NIM_ADD, &nid)) {
        g_trayAdded = true;
    }
}

void RemoveTrayIcon() {
    if (!g_trayAdded) return;

    NOTIFYICONDATAA nid{};
    nid.cbSize = sizeof(nid);
    nid.hWnd = g_hwndHidden;
    nid.uID = ID_TRAYICON;
    Shell_NotifyIconA(NIM_DELETE, &nid);
    g_trayAdded = false;
}

void UpdateTrayState() {
    if (g_paused) SetTrayTip("LapScroll - paused");
    else if (g_scrollMode) SetTrayTip("LapScroll - scrolling");
    else SetTrayTip("LapScroll - running");
}

bool RegisterToggleHotkey(HWND hwnd) {
    UnregisterHotKey(hwnd, ID_HOTKEY);

    if (g_cfg.hotkeyKey == VK_SHIFT || g_cfg.hotkeyKey == VK_CONTROL || g_cfg.hotkeyKey == VK_MENU ||
        g_cfg.hotkeyKey == VK_LWIN || g_cfg.hotkeyKey == VK_RWIN) {
        return false;
    }

    return RegisterHotKey(hwnd, ID_HOTKEY, g_cfg.hotkeyModifiers, g_cfg.hotkeyKey) != FALSE;
}

void StartScrollMode() {
    GetCursorPos(&g_lockPos);
    g_lastPos = g_lockPos;
    g_scrollMode = true;
    UpdateTrayState();
}

void StopScrollMode() {
    g_scrollMode = false;
    UpdateTrayState();
}

void ToggleScrollMode() {
    if (g_scrollMode) StopScrollMode();
    else StartScrollMode();
}

void SendWheel(int wheelAmount) {
    INPUT in{};
    in.type = INPUT_MOUSE;
    in.mi.dwFlags = MOUSEEVENTF_WHEEL;
    in.mi.mouseData = wheelAmount;
    SendInput(1, &in, sizeof(INPUT));
}

void DoScrollStep() {
    if (!g_scrollMode || g_paused) return;

    POINT currentPos{};
    GetCursorPos(&currentPos);

    int deltaY = currentPos.y - g_lastPos.y;

    if (std::abs(deltaY) > g_cfg.deadzone) {
        int wheelAmount = -deltaY * g_cfg.scrollSpeed;

        mouse_event(
            MOUSEEVENTF_WHEEL,
            0,
            0,
            wheelAmount,
            0
        );
    }

    SetCursorPos(g_lockPos.x, g_lockPos.y);
    g_lastPos = g_lockPos;
}

void ReloadSettings() {
    LoadConfigFromFile(g_cfg, GetSettingsPathA());
    SetAutostartEnabled(g_cfg.autostart);
    RegisterToggleHotkey(g_hwndHidden);
    UpdateTrayState();
}

void ShowTrayMenu(HWND hwnd) {
    HMENU menu = CreatePopupMenu();
    if (!menu) return;

    AppendMenuA(menu, MF_STRING, IDM_OPEN_SETTINGS, "Open Settings");
    AppendMenuA(menu, MF_STRING, IDM_TOGGLE_PAUSE, g_paused ? "Resume" : "Pause");
    AppendMenuA(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuA(menu, MF_STRING, IDM_RELOAD, "Reload Settings");
    AppendMenuA(menu, MF_STRING, IDM_EXIT_APP, "Exit");

    POINT pt{};
    GetCursorPos(&pt);
    SetForegroundWindow(hwnd);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_LEFTALIGN,
                   pt.x, pt.y, 0, hwnd, nullptr);
    DestroyMenu(menu);
}

// --------------------------------------------------
// Window proc
// --------------------------------------------------
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        LoadConfigFromFile(g_cfg, GetSettingsPathA());
        SetAutostartEnabled(g_cfg.autostart);
        AddTrayIcon();
        UpdateTrayState();

        if (!RegisterToggleHotkey(hwnd)) {
            MessageBoxA(nullptr, "Hotkey register failed. Change the hotkey in settings.", "LapScroll", MB_ICONWARNING);
        }

        SetTimer(hwnd, ID_TIMER, 5, nullptr);

        if (!FileExistsA_(GetSettingsPathA().c_str())) {
            LaunchSettingsApp();
        }
        return 0;

    case WM_LAPSCROLL_RELOAD:
        ReloadSettings();
        return 0;

    case WM_LAPSCROLL_TRAY:
        if (lParam == WM_RBUTTONUP || lParam == WM_CONTEXTMENU) {
            ShowTrayMenu(hwnd);
        } else if (lParam == WM_LBUTTONUP) {
            ToggleScrollMode();
        }
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDM_OPEN_SETTINGS:
            LaunchSettingsApp();
            return 0;
        case IDM_TOGGLE_PAUSE:
            g_paused = !g_paused;
            if (g_paused) StopScrollMode();
            UpdateTrayState();
            return 0;
        case IDM_RELOAD:
            ReloadSettings();
            return 0;
        case IDM_EXIT_APP:
            DestroyWindow(hwnd);
            return 0;
        }
        break;

    case WM_HOTKEY:
        if ((int)wParam == ID_HOTKEY) {
            ToggleScrollMode();
        }
        return 0;

    case WM_TIMER:
        if ((int)wParam == ID_TIMER) {
            DoScrollStep();
        }
        return 0;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd, ID_TIMER);
        UnregisterHotKey(hwnd, ID_HOTKEY);
        RemoveTrayIcon();
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

// --------------------------------------------------
// WinMain
// --------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    g_hInst = hInstance;

    HANDLE hMutex = CreateMutexA(nullptr, TRUE, kMutexName);
    if (!hMutex || GetLastError() == ERROR_ALREADY_EXISTS) {
        if (hMutex) CloseHandle(hMutex);
        return 0;
    }

    WNDCLASSA wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = kBackgroundWindowClass;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    if (!RegisterClassA(&wc)) {
        MessageBoxA(nullptr, "Failed to register background window class.", "LapScroll", MB_ICONERROR);
        CloseHandle(hMutex);
        return 1;
    }

    g_hwndHidden = CreateWindowExA(
        0,
        kBackgroundWindowClass,
        "LapScrollHidden",
        0,
        0, 0, 0, 0,
        HWND_MESSAGE,
        nullptr,
        hInstance,
        nullptr
    );

    if (!g_hwndHidden) {
        MessageBoxA(nullptr, "Failed to create hidden window.", "LapScroll", MB_ICONERROR);
        CloseHandle(hMutex);
        return 1;
    }

    MSG msg{};
    while (GetMessageA(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
    return 0;
}