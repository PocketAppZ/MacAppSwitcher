#include <windef.h>
#include <stdbool.h>

typedef struct KeyConfig
{
    DWORD _AppHold;
    DWORD _AppSwitch;
    DWORD _WinHold;
    DWORD _WinSwitch;
    DWORD _Invert;
    DWORD _PrevApp;
} KeyConfig;

typedef enum ThemeMode
{
    ThemeModeAuto,
    ThemeModeLight,
    ThemeModeDark
} ThemeMode;

typedef enum AppSwitcherMode
{
    AppSwitcherModeApp,
    AppSwitcherModeWindow,
} AppSwitcherMode;

typedef struct Config
{
    KeyConfig _Key;
    bool _Mouse;
    bool _CheckForUpdates;
    ThemeMode _ThemeMode;
    float _Scale;
    AppSwitcherMode _AppSwitcherMode;
} Config;

void LoadConfig(Config* config);