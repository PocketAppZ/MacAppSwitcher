#include "Settings.h"
#include <minwindef.h>
#include <stdio.h>
#include <windef.h>
#include <windows.h>
#include <windowsx.h>
#include <winuser.h>
#include <commctrl.h>
#include <debugapi.h>
#include <process.h>
#include <Tlhelp32.h>

#include "Config/Config.h"
#include "Error/Error.h"

static const char CLASS_NAME[] = "AltAppSwitcherSettings";

typedef struct EnumBinding
{
    unsigned int* _TargetValue;
    HWND _ComboBox;
    const EnumString* _EnumStrings;
} EnumBinding;

typedef struct FloatBinding
{
    float* _TargetValue;
    HWND _Field;
} FloatBinding;

typedef struct AppData
{
    EnumBinding _EBindings[64];
    unsigned int _EBindingCount;
    FloatBinding _FBindings[64];
    unsigned int _FBindingCount;
    Config _Config;
    HFONT _Font;
} AppData;

#define WIN_PAD 10

static void CreateTooltip(HWND parent, HWND tool, char* string)
{
    HWND tt = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
        WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
        0, 0, 100, 100,
        parent, NULL, (HINSTANCE)GetWindowLongPtr(parent, GWLP_HINSTANCE), NULL);

    SetWindowPos(tt, HWND_TOPMOST, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    TOOLINFO ti = {};
    ti.cbSize = sizeof(TOOLINFO);
    ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    ti.uId = (UINT_PTR)tool;
    ti.lpszText = string;
    ti.hwnd = parent;

    SendMessage(tt, TTM_ADDTOOL, 0, (LPARAM)&ti);
    SendMessage(tt, TTM_ACTIVATE, true, (LPARAM)NULL);
}

static void CreateLabel(int x, int y, int width, int height, HWND parent, const char* name, const char* tooltip, AppData* appData)
{
    HINSTANCE inst = (HINSTANCE)GetWindowLongPtrA(parent, GWLP_HINSTANCE);
    HWND label = CreateWindow(WC_STATIC, name,
        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE | SS_NOTIFY, // notify needed to tooltip
        x, y, width, height,
        parent, NULL, inst, NULL);
    SendMessage(label, WM_SETFONT, (WPARAM)appData->_Font, true);
    CreateTooltip(parent, label, (char*)tooltip);
}

static void CreateFloatField(int x, int y, int w, int h, HWND parent, const char* name, const char* tooltip, float* value, AppData* appData)
{
    CreateLabel(x, y, w / 2, h, parent, name, tooltip, appData);
    HINSTANCE inst = (HINSTANCE)GetWindowLongPtrA(parent, GWLP_HINSTANCE);
    char sval[4] = "000";
    sprintf(sval, "%3d", (int)(*value * 100));
    HWND field = CreateWindow(WC_EDIT, sval,
        WS_CHILD | WS_VISIBLE | ES_LEFT | ES_CENTER | ES_NUMBER,
        x + w / 2, y, w / 2, h,
        parent, NULL, inst, NULL);
    SendMessage(field, WM_SETFONT, (WPARAM)appData->_Font, true);
    SendMessage(field, EM_LIMITTEXT, (WPARAM)3, true);

    appData->_FBindings[appData->_FBindingCount]._Field = field;
    appData->_FBindings[appData->_FBindingCount]._TargetValue = value;
    appData->_FBindingCount++;
}
static void CreateComboBox(int x, int y, int w, int h, HWND parent, const char* name, const char* tooltip, unsigned int* value, const EnumString* enumStrings, AppData* appData)
{
    CreateLabel(x, y, w / 2, h, parent, name, tooltip, appData);

    HINSTANCE inst = (HINSTANCE)GetWindowLongPtrA(parent, GWLP_HINSTANCE);
    HWND combobox = CreateWindow(WC_COMBOBOX, "Combobox",
        CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_VISIBLE,
        x + w / 2, y, w / 2, h,
        parent, NULL, inst, NULL);
    for (unsigned int i = 0; enumStrings[i].Value != 0xFFFFFFFF; i++)
    {
        SendMessage(combobox,(UINT)CB_ADDSTRING,(WPARAM)0,(LPARAM)enumStrings[i].Name);
        if (*value == enumStrings[i].Value)
            SendMessage(combobox,(UINT)CB_SETCURSEL,(WPARAM)i, (LPARAM)0);
    }
    SendMessage(combobox, WM_SETFONT, (WPARAM)appData->_Font, true);
    CreateTooltip(parent, combobox, (char*)tooltip);

    appData->_EBindings[appData->_EBindingCount]._ComboBox = combobox;
    appData->_EBindings[appData->_EBindingCount]._EnumStrings = enumStrings;
    appData->_EBindings[appData->_EBindingCount]._TargetValue = value;
    appData->_EBindingCount++;
}

void CreateButton(int x, int y, int w, int h, HWND parent, const char* name, HMENU ID, AppData* appData)
{
    (void)w;
    HINSTANCE inst = (HINSTANCE)GetWindowLongPtrA(parent, GWLP_HINSTANCE);
    HWND button = CreateWindow(WC_BUTTON, name,
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_FLAT,
        x, y, 0, 0, parent, (HMENU)ID, inst, NULL);
    SendMessage(button, WM_SETFONT, (WPARAM)appData->_Font, true);
    SIZE size = {};
    Button_GetIdealSize(button, &size);
    SetWindowPos(button, NULL, 0, 0, size.cx, h, SWP_NOMOVE);
}

static bool KillAAS()
{
    HANDLE hSnapShot = CreateToolhelp32Snapshot((DWORD)TH32CS_SNAPALL, (DWORD)0);
    PROCESSENTRY32 pEntry;
    pEntry.dwSize = sizeof (pEntry);
    BOOL hRes = Process32First(hSnapShot, &pEntry);
    BOOL killed = false;
    while (hRes)
    {
        if (strcmp(pEntry.szExeFile, "AltAppSwitcher.exe") == 0)
        {
            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, 0,
                (DWORD) pEntry.th32ProcessID);
            if (hProcess != NULL)
            {
                TerminateProcess(hProcess, 9);
                CloseHandle(hProcess);
                killed |= true;
            }
        }
        hRes = Process32Next(hSnapShot, &pEntry);
    }
    CloseHandle(hSnapShot);
    return killed;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static AppData appData = {};

#define APPLY_BUTTON_ID 1993
    switch (uMsg)
    {
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        DeleteFont(appData._Font);
        appData._Font = NULL;
        return 0;
    }
    case WM_CREATE:
    {
        LoadConfig(&appData._Config);

        {
            NONCLIENTMETRICS metrics = {};
            metrics.cbSize = sizeof(metrics);
            SystemParametersInfo(SPI_GETNONCLIENTMETRICS, metrics.cbSize, &metrics, 0);
            metrics.lfCaptionFont.lfHeight *= 1.2;
            metrics.lfCaptionFont.lfWidth *= 1.2;
            appData._Font = CreateFontIndirect(&metrics.lfCaptionFont);
        }

        int x = WIN_PAD;
        int y = WIN_PAD;
        int h = 0;
        {
            HWND combobox = CreateWindow(WC_COMBOBOX, "Combobox",
                CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_VISIBLE,
                0, 0, 0, 0,
                hwnd, NULL, NULL, NULL);
            SendMessage(combobox, WM_SETFONT, (LPARAM)appData._Font, true);
             RECT rect = {};
            GetWindowRect(combobox, &rect);
            h = rect.bottom -  rect.top;
            DestroyWindow(combobox);
        }
        int w = 0;
        {
            RECT parentRect = {};
            GetClientRect(hwnd, &parentRect);
            w = (parentRect.right - parentRect.left - WIN_PAD - WIN_PAD);
        }

        CreateComboBox(x, y, w, h, hwnd, "Theme", "Color scheme. \"Auto\" to match system's.", &appData._Config._ThemeMode, themeES, &appData);
        y += h;
        CreateComboBox(x, y, w, h, hwnd, "App hold key", "App hold key", &appData._Config._Key._AppHold, keyES, &appData);
        y += h;
        CreateComboBox(x, y, w, h, hwnd, "Switcher mode",
            "App: MacOS-like, one entry per application. Window: Windows-like, one entry per window (each window is considered an independent application)",
            &appData._Config._AppSwitcherMode, appSwitcherModeES, &appData);
        y += h;
        CreateFloatField(x, y, w, h, hwnd, "Scale",
            " Scale controls icon size, expressed as percentage, 100 being Windows default icon size.",
            &appData._Config._Scale, &appData);
        y += h;
        CreateButton(x, y, w, h, hwnd, "Apply", (HMENU)APPLY_BUTTON_ID, &appData);

        return 0;
    }
    case WM_COMMAND:
    {
        if (LOWORD(wParam == APPLY_BUTTON_ID) && HIWORD(wParam) == BN_CLICKED)
        {
            for (unsigned int i = 0; i < appData._EBindingCount; i++)
            {
                const EnumBinding* bd = &appData._EBindings[i];

                const unsigned int iValue = SendMessage(bd->_ComboBox,(UINT)CB_GETCURSEL,(WPARAM)0, (LPARAM)0);
                char sValue[64] = {};
                SendMessage(bd->_ComboBox,(UINT)CB_GETLBTEXT,(WPARAM)iValue, (LPARAM)sValue);
                bool found = false;
                for (unsigned int j = 0; bd->_EnumStrings[j].Value != 0xFFFFFFFF; j++)
                {
                    if (!strcmp(bd->_EnumStrings[j].Name, sValue))
                    {
                        *bd->_TargetValue = bd->_EnumStrings[j].Value;
                        found = true;
                        break;
                    }
                }
                ASSERT(found);
            }
            for (unsigned int i = 0; i < appData._FBindingCount; i++)
            {
                const FloatBinding* bd = &appData._FBindings[i];
                char text[4] = "000";
                *((DWORD*)text) = 3;
                SendMessage(bd->_Field,(UINT)EM_GETLINE,(WPARAM)0, (LPARAM)text);
                *bd->_TargetValue = (float)strtod(text, NULL) / 100.0f;
            }
            WriteConfig(&appData._Config);
            if (KillAAS())
                system("start .\\AltAppSwitcher.exe");
        }
        return 0;
    }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int StartSettings(HINSTANCE hInstance)
{
    INITCOMMONCONTROLSEX ic;
    ic.dwSize = sizeof(INITCOMMONCONTROLSEX);
    ic.dwICC = ICC_TAB_CLASSES;
    InitCommonControlsEx(&ic);

    // Main window
    {
        // Class
        WNDCLASS wc = { };
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = CLASS_NAME;
        wc.cbWndExtra = 0;
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.hbrBackground = GetSysColorBrush(COLOR_HIGHLIGHT);
        RegisterClass(&wc);
        // Window
        const int center[2] = { GetSystemMetrics(SM_CXSCREEN) / 2, GetSystemMetrics(SM_CYSCREEN) / 2 };
        DWORD winStyle = WS_CAPTION | WS_SYSMENU | WS_BORDER | WS_VISIBLE | WS_MINIMIZEBOX;
        RECT winRect = { center[0] - 200, center[1] - 300, center[0] + 200, center[1] + 300 };
        AdjustWindowRect(&winRect, winStyle, false);
        CreateWindow(CLASS_NAME, "Alt App Switcher settings",
            winStyle,
            winRect.left, winRect.top, winRect.right - winRect.left, winRect.bottom - winRect.top,
            NULL, NULL, hInstance, NULL);
    }

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    UnregisterClass(CLASS_NAME, hInstance);

    return 0;
}