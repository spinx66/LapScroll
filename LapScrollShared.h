#pragma once
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <cmath>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")

// --------------------------------------------------
// Config
// --------------------------------------------------
struct LapScrollConfig {
    UINT hotkeyModifiers = MOD_CONTROL | MOD_SHIFT;
    UINT hotkeyKey = 'S';
    int  scrollSpeed = 5;
    int  deadzone = 1;
    bool autostart = false;
};

static constexpr const char* kBackgroundWindowClass = "LapScrollHiddenWindow";
static constexpr const char* kSettingsWindowClass   = "LapScrollSettingsWindow";
static constexpr const char* kRunValueName          = "LapScroll";
static constexpr UINT        WM_LAPSCROLL_TRAY      = WM_APP + 2;
static constexpr UINT        WM_LAPSCROLL_RELOAD    = WM_APP + 3;

// --------------------------------------------------
// Small string helpers
// --------------------------------------------------
inline std::string Trim(std::string s) {
    auto notSpace = [](unsigned char ch) { return !std::isspace(ch); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
    s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
    return s;
}

inline std::string ToLower(std::string s) {
    for (char& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}

inline bool FileExistsA_(const char* path) {
    DWORD attr = GetFileAttributesA(path);
    return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

inline std::string GetExePathA() {
    char buf[MAX_PATH]{};
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    return buf;
}

inline std::string GetExeDirA() {
    std::string p = GetExePathA();
    size_t pos = p.find_last_of("\\/");
    if (pos == std::string::npos) return ".\\";
    return p.substr(0, pos + 1);
}

inline std::string GetSettingsPathA() {
    return GetExeDirA() + "settings.json";
}

inline std::string GetBackgroundExePathA() {
    return GetExeDirA() + "LapScroll.exe";
}

inline std::string GetSettingsExePathA() {
    return GetExeDirA() + "LapScroll.Settings.exe";
}

// --------------------------------------------------
// Hotkey text helpers
// --------------------------------------------------
inline std::string VkToText(UINT vk) {
    if (vk >= VK_F1 && vk <= VK_F24) return "F" + std::to_string(vk - VK_F1 + 1);
    if (vk >= 'A' && vk <= 'Z') return std::string(1, (char)vk);
    if (vk >= '0' && vk <= '9') return std::string(1, (char)vk);

    switch (vk) {
    case VK_ESCAPE:  return "Esc";
    case VK_SPACE:   return "Space";
    case VK_TAB:     return "Tab";
    case VK_RETURN:  return "Enter";
    case VK_BACK:    return "Backspace";
    case VK_DELETE:  return "Delete";
    case VK_INSERT:  return "Insert";
    case VK_HOME:    return "Home";
    case VK_END:     return "End";
    case VK_PRIOR:   return "PageUp";
    case VK_NEXT:    return "PageDown";
    case VK_LEFT:    return "Left";
    case VK_RIGHT:   return "Right";
    case VK_UP:      return "Up";
    case VK_DOWN:    return "Down";
    case VK_OEM_3:   return "`";
    default:         return "VK_" + std::to_string(vk);
    }
}

inline std::string HotkeyToText(UINT mods, UINT key) {
    std::string out;
    if (mods & MOD_CONTROL) out += "Ctrl + ";
    if (mods & MOD_SHIFT)   out += "Shift + ";
    if (mods & MOD_ALT)     out += "Alt + ";
    if (mods & MOD_WIN)     out += "Win + ";
    out += VkToText(key);
    return out;
}

inline bool ParseKeyToken(const std::string& token, UINT& keyOut) {
    std::string t = ToLower(Trim(token));

    if (t.size() == 1) {
        char c = t[0];
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
            keyOut = (UINT)std::toupper((unsigned char)c);
            return true;
        }
    }

    if (t.size() >= 2 && t[0] == 'f') {
        int n = std::atoi(t.c_str() + 1);
        if (n >= 1 && n <= 24) {
            keyOut = VK_F1 + (n - 1);
            return true;
        }
    }

    if (t == "esc" || t == "escape")   { keyOut = VK_ESCAPE; return true; }
    if (t == "space")                  { keyOut = VK_SPACE;   return true; }
    if (t == "tab")                    { keyOut = VK_TAB;     return true; }
    if (t == "enter" || t == "return") { keyOut = VK_RETURN;  return true; }
    if (t == "backspace")              { keyOut = VK_BACK;    return true; }
    if (t == "delete" || t == "del")   { keyOut = VK_DELETE;  return true; }
    if (t == "insert" || t == "ins")   { keyOut = VK_INSERT;  return true; }
    if (t == "home")                   { keyOut = VK_HOME;    return true; }
    if (t == "end")                    { keyOut = VK_END;     return true; }
    if (t == "pageup" || t == "pgup")   { keyOut = VK_PRIOR;   return true; }
    if (t == "pagedown" || t == "pgdn") { keyOut = VK_NEXT;    return true; }
    if (t == "left")                   { keyOut = VK_LEFT;    return true; }
    if (t == "right")                  { keyOut = VK_RIGHT;   return true; }
    if (t == "up")                     { keyOut = VK_UP;      return true; }
    if (t == "down")                   { keyOut = VK_DOWN;    return true; }
    if (t == "backtick" || t == "`")   { keyOut = VK_OEM_3;   return true; }

    return false;
}

inline bool ParseHotkeyText(const std::string& text, UINT& modsOut, UINT& keyOut) {
    modsOut = 0;
    keyOut = 0;

    std::stringstream ss(text);
    std::string part;
    std::string lastKeyToken;
    bool foundKey = false;

    while (std::getline(ss, part, '+')) {
        std::string t = ToLower(Trim(part));
        if (t.empty()) continue;

        if (t == "ctrl" || t == "control") {
            modsOut |= MOD_CONTROL;
        } else if (t == "shift") {
            modsOut |= MOD_SHIFT;
        } else if (t == "alt") {
            modsOut |= MOD_ALT;
        } else if (t == "win" || t == "meta" || t == "super") {
            modsOut |= MOD_WIN;
        } else {
            lastKeyToken = part;
            foundKey = true;
        }
    }

    if (!foundKey) return false;
    return ParseKeyToken(lastKeyToken, keyOut);
}

inline UINT CaptureModifiersFromKeyboard() {
    UINT mods = 0;

    if (GetAsyncKeyState(VK_CONTROL) & 0x8000) mods |= MOD_CONTROL;
    if (GetAsyncKeyState(VK_SHIFT) & 0x8000)   mods |= MOD_SHIFT;
    if (GetAsyncKeyState(VK_MENU) & 0x8000)    mods |= MOD_ALT;
    if (GetAsyncKeyState(VK_LWIN) & 0x8000 || GetAsyncKeyState(VK_RWIN) & 0x8000) mods |= MOD_WIN;

    return mods;
}

// --------------------------------------------------
// File helpers
// --------------------------------------------------
inline std::string ReadFileTextA_(const char* path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return {};
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

inline bool WriteFileTextA_(const char* path, const std::string& text) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) return false;
    file << text;
    return true;
}

// --------------------------------------------------
// Tiny JSON-ish parser for our own config
// --------------------------------------------------
inline bool FindQuotedStringField(const std::string& text, const std::string& key, std::string& out) {
    const std::string needle = "\"" + key + "\"";
    size_t pos = text.find(needle);
    if (pos == std::string::npos) return false;

    pos = text.find(':', pos);
    if (pos == std::string::npos) return false;

    pos = text.find('"', pos);
    if (pos == std::string::npos) return false;

    size_t end = pos + 1;
    while (end < text.size()) {
        if (text[end] == '"' && text[end - 1] != '\\') break;
        ++end;
    }
    if (end >= text.size()) return false;

    out = text.substr(pos + 1, end - pos - 1);
    return true;
}

inline int FindIntField(const std::string& text, const std::string& key, int fallback) {
    size_t pos = text.find("\"" + key + "\"");
    if (pos == std::string::npos) return fallback;

    pos = text.find(':', pos);
    if (pos == std::string::npos) return fallback;

    ++pos;
    while (pos < text.size() && std::isspace((unsigned char)text[pos])) ++pos;

    size_t end = pos;
    while (end < text.size() && (std::isdigit((unsigned char)text[end]) || text[end] == '-')) ++end;

    try {
        return std::stoi(text.substr(pos, end - pos));
    } catch (...) {
        return fallback;
    }
}

inline bool FindBoolField(const std::string& text, const std::string& key, bool fallback) {
    std::string lower = ToLower(text);
    std::string needle = "\"" + ToLower(key) + "\"";
    size_t pos = lower.find(needle);
    if (pos == std::string::npos) return fallback;

    pos = lower.find(':', pos);
    if (pos == std::string::npos) return fallback;

    ++pos;
    while (pos < lower.size() && std::isspace((unsigned char)lower[pos])) ++pos;

    if (lower.compare(pos, 4, "true") == 0)  return true;
    if (lower.compare(pos, 5, "false") == 0) return false;
    if (lower.compare(pos, 1, "1") == 0)     return true;
    if (lower.compare(pos, 1, "0") == 0)     return false;

    return fallback;
}

inline bool LoadConfigFromFile(LapScrollConfig& cfg, const std::string& path) {
    std::string json = ReadFileTextA_(path.c_str());
    if (json.empty()) return false;

    std::string hotkeyText;
    if (FindQuotedStringField(json, "hotkey", hotkeyText)) {
        UINT mods = 0, key = 0;
        if (ParseHotkeyText(hotkeyText, mods, key)) {
            cfg.hotkeyModifiers = mods;
            cfg.hotkeyKey = key;
        }
    } else {
        int mods = FindIntField(json, "hotkeyModifiers", (int)(MOD_CONTROL | MOD_SHIFT));
        int key  = FindIntField(json, "hotkeyKey", 'S');
        cfg.hotkeyModifiers = (UINT)std::max(0, mods);
        cfg.hotkeyKey = (UINT)std::max(0, key);
    }

    cfg.scrollSpeed = std::max(1, FindIntField(json, "scrollSpeed", 5));
    cfg.deadzone    = std::max(0, FindIntField(json, "deadzone", 1));
    cfg.autostart   = FindBoolField(json, "autostart", false);
    return true;
}

inline bool SaveConfigToFile(const LapScrollConfig& cfg, const std::string& path) {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"hotkey\": \"" << HotkeyToText(cfg.hotkeyModifiers, cfg.hotkeyKey) << "\",\n";
    ss << "  \"hotkeyModifiers\": " << cfg.hotkeyModifiers << ",\n";
    ss << "  \"hotkeyKey\": " << cfg.hotkeyKey << ",\n";
    ss << "  \"scrollSpeed\": " << cfg.scrollSpeed << ",\n";
    ss << "  \"deadzone\": " << cfg.deadzone << ",\n";
    ss << "  \"autostart\": " << (cfg.autostart ? "true" : "false") << "\n";
    ss << "}\n";
    return WriteFileTextA_(path.c_str(), ss.str());
}

// --------------------------------------------------
// Startup registry helpers
// --------------------------------------------------
inline bool SetAutostartEnabled(bool enabled) {
    std::string exe = GetBackgroundExePathA();
    std::string quoted = "\"" + exe + "\"";

    HKEY key{};
    if (RegOpenKeyExA(HKEY_CURRENT_USER,
        "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, KEY_SET_VALUE, &key) != ERROR_SUCCESS) {
        return false;
    }

    LONG result = ERROR_SUCCESS;
    if (enabled) {
        result = RegSetValueExA(key, kRunValueName, 0, REG_SZ,
            (const BYTE*)quoted.c_str(), (DWORD)(quoted.size() + 1));
    } else {
        result = RegDeleteValueA(key, kRunValueName);
    }

    RegCloseKey(key);
    return result == ERROR_SUCCESS;
}

inline bool GetAutostartEnabled() {
    HKEY key{};
    if (RegOpenKeyExA(HKEY_CURRENT_USER,
        "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS) {
        return false;
    }

    char buf[1024]{};
    DWORD type = 0;
    DWORD size = sizeof(buf);
    LONG result = RegQueryValueExA(key, kRunValueName, nullptr, &type, (LPBYTE)buf, &size);
    RegCloseKey(key);

    if (result != ERROR_SUCCESS || type != REG_SZ) return false;

    std::string v = ToLower(buf);
    std::string exe = ToLower(GetBackgroundExePathA());
    return v.find(exe) != std::string::npos;
}

// --------------------------------------------------
// Cross-process helpers
// --------------------------------------------------
inline void LaunchSettingsApp() {
    std::string settingsExe = GetSettingsExePathA();
    ShellExecuteA(nullptr, "open", settingsExe.c_str(), nullptr, GetExeDirA().c_str(), SW_SHOWNORMAL);
}

inline void NotifyBackgroundReload() {
    HWND hwnd = FindWindowA(kBackgroundWindowClass, nullptr);
    if (hwnd) PostMessageA(hwnd, WM_LAPSCROLL_RELOAD, 0, 0);
}