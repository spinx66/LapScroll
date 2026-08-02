#include <windows.h>
#include <shellapi.h>
#include <string>
#include "LapScrollShared.h"
#define IDI_ICON1 101

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")

// --------------------------------------------------
// IDs
// --------------------------------------------------
constexpr int ID_BTN_CHANGEKEY = 1001;
constexpr int ID_BTN_APPLY      = 1002;
constexpr int ID_BTN_CLOSE      = 1003;
constexpr int ID_EDIT_SPEED     = 1004;
constexpr int ID_EDIT_DEADZONE   = 1005;
constexpr int ID_LBL_HOTKEY     = 1006;
constexpr int ID_LBL_STATUS     = 1007;
constexpr int ID_CHK_AUTOSTART   = 1008;

// --------------------------------------------------
// Globals
// --------------------------------------------------
HINSTANCE g_hInst = nullptr;
HWND g_hwndMain = nullptr;
HWND g_lblHotkey = nullptr;
HWND g_lblStatus = nullptr;
HWND g_editSpeed = nullptr;
HWND g_editDeadzone = nullptr;
HWND g_chkAutostart = nullptr;

LapScrollConfig g_cfg{};
bool g_waitingForHotkey = false;

const char* kMutexName = "LapScrollSettingsSingleton";

// --------------------------------------------------
// Helpers
// --------------------------------------------------
void SetStatusText(const std::string& text) {
    if (g_lblStatus) SetWindowTextA(g_lblStatus, text.c_str());
}

void UpdateHotkeyLabel() {
    if (!g_lblHotkey) return;
    std::string s = "Current hotkey: " + HotkeyToText(g_cfg.hotkeyModifiers, g_cfg.hotkeyKey);
    SetWindowTextA(g_lblHotkey, s.c_str());
}

void WriteSettingsToUI() {
    SetWindowTextA(g_editSpeed, std::to_string(g_cfg.scrollSpeed).c_str());
    SetWindowTextA(g_editDeadzone, std::to_string(g_cfg.deadzone).c_str());
    SendMessageA(g_chkAutostart, BM_SETCHECK, g_cfg.autostart ? BST_CHECKED : BST_UNCHECKED, 0);
    UpdateHotkeyLabel();
}

int ReadEditInt(HWND hEdit, int fallback, int minValue) {
    char buf[64]{};
    GetWindowTextA(hEdit, buf, sizeof(buf));
    int v = fallback;
    try {
        v = std::stoi(buf);
    } catch (...) {
        v = fallback;
    }
    return (v < minValue) ? minValue : v;
}

void ReadSettingsFromUI() {
    g_cfg.scrollSpeed = ReadEditInt(g_editSpeed, 5, 1);
    g_cfg.deadzone = ReadEditInt(g_editDeadzone, 1, 0);
    g_cfg.autostart = (SendMessageA(g_chkAutostart, BM_GETCHECK, 0, 0) == BST_CHECKED);
}

void SaveAndNotify() {
    ReadSettingsFromUI();
    SaveConfigToFile(g_cfg, GetSettingsPathA());
    SetAutostartEnabled(g_cfg.autostart);
    NotifyBackgroundReload();
}

void BeginHotkeyCapture() {
    g_waitingForHotkey = true;
    SetStatusText("Press a normal key now...");
    SetFocus(g_hwndMain);
}

void BringExistingSettingsToFront() {
    HWND existing = FindWindowA(kSettingsWindowClass, nullptr);
    if (existing) {
        ShowWindow(existing, SW_SHOWNORMAL);
        SetForegroundWindow(existing);
    }
}

// --------------------------------------------------
// Window proc
// --------------------------------------------------
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        CreateWindowA("STATIC", "LapScroll Settings",
            WS_CHILD | WS_VISIBLE,
            20, 15, 220, 20, hwnd, nullptr, g_hInst, nullptr);

        g_lblHotkey = CreateWindowA("STATIC", "Current hotkey: Ctrl + Shift + S",
            WS_CHILD | WS_VISIBLE,
            20, 45, 260, 20, hwnd, (HMENU)ID_LBL_HOTKEY, g_hInst, nullptr);

        CreateWindowA("BUTTON", "Change hotkey",
            WS_CHILD | WS_VISIBLE,
            20, 75, 120, 28, hwnd, (HMENU)ID_BTN_CHANGEKEY, g_hInst, nullptr);

        CreateWindowA("STATIC", "Scroll speed:",
            WS_CHILD | WS_VISIBLE,
            20, 120, 90, 20, hwnd, nullptr, g_hInst, nullptr);

        g_editSpeed = CreateWindowA("EDIT", "5",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
            120, 117, 80, 24, hwnd, (HMENU)ID_EDIT_SPEED, g_hInst, nullptr);

        CreateWindowA("STATIC", "Deadzone:",
            WS_CHILD | WS_VISIBLE,
            20, 155, 90, 20, hwnd, nullptr, g_hInst, nullptr);

        g_editDeadzone = CreateWindowA("EDIT", "1",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
            120, 152, 80, 24, hwnd, (HMENU)ID_EDIT_DEADZONE, g_hInst, nullptr);

        g_chkAutostart = CreateWindowA("BUTTON", "Run at startup",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            20, 190, 160, 24, hwnd, (HMENU)ID_CHK_AUTOSTART, g_hInst, nullptr);

        CreateWindowA("BUTTON", "Apply",
            WS_CHILD | WS_VISIBLE,
            20, 230, 90, 28, hwnd, (HMENU)ID_BTN_APPLY, g_hInst, nullptr);

        CreateWindowA("BUTTON", "Close",
            WS_CHILD | WS_VISIBLE,
            120, 230, 90, 28, hwnd, (HMENU)ID_BTN_CLOSE, g_hInst, nullptr);

        g_lblStatus = CreateWindowA("STATIC", "Ready",
            WS_CHILD | WS_VISIBLE,
            20, 270, 280, 20, hwnd, (HMENU)ID_LBL_STATUS, g_hInst, nullptr);

        LoadConfigFromFile(g_cfg, GetSettingsPathA());
        if (g_cfg.scrollSpeed <= 0) g_cfg.scrollSpeed = 5;
        if (g_cfg.deadzone < 0) g_cfg.deadzone = 1;
        g_cfg.autostart = GetAutostartEnabled();

        WriteSettingsToUI();
        SetStatusText("Loaded");
        return 0;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_BTN_CHANGEKEY:
            BeginHotkeyCapture();
            return 0;

        case ID_BTN_APPLY:
            SaveAndNotify();
            WriteSettingsToUI();
            SetStatusText("Saved");
            return 0;

        case ID_BTN_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        }
        break;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (g_waitingForHotkey) {
            UINT vk = (UINT)wParam;

            if (vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU ||
                vk == VK_LWIN || vk == VK_RWIN) {
                SetStatusText("Pick a normal key as the last key.");
                return 0;
            }

            g_cfg.hotkeyModifiers = CaptureModifiersFromKeyboard();
            g_cfg.hotkeyKey = vk;
            g_waitingForHotkey = false;

            UpdateHotkeyLabel();
            SaveAndNotify();
            SetStatusText("Hotkey saved");
            return 0;
        }
        break;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

// --------------------------------------------------
// WinMain
// --------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    g_hInst = hInstance;

    HANDLE hMutex = CreateMutexA(nullptr, TRUE, kMutexName);
    if (!hMutex || GetLastError() == ERROR_ALREADY_EXISTS) {
        if (hMutex) CloseHandle(hMutex);
        BringExistingSettingsToFront();
        return 0;
    }

    WNDCLASSA wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = kSettingsWindowClass;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClassA(&wc)) {
        MessageBoxA(nullptr, "Failed to register settings window class.", "LapScroll", MB_ICONERROR);
        CloseHandle(hMutex);
        return 1;
    }

    g_hwndMain = CreateWindowA(
        kSettingsWindowClass,
        "LapScroll Settings",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 340, 340,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!g_hwndMain) {
        MessageBoxA(nullptr, "Failed to create settings window.", "LapScroll", MB_ICONERROR);
        CloseHandle(hMutex);
        return 1;
    }

    ShowWindow(g_hwndMain, nCmdShow);
    UpdateWindow(g_hwndMain);

    MSG msg{};
    while (GetMessageA(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
    return 0;
}
