#include "AltAppSwitcher/Config.h"
#include "AltAppSwitcher/_Generated/ConfigStr.h"
#include "AltAppSwitcher/KeyCodeFromConfigName.h"
#include <stdbool.h>
#include <stdio.h>
#include <Winuser.h>
#include <stdlib.h>

static bool TryGetKey(const char* lineBuf, const char* token, DWORD* keyToSet)
{
    const char* pValue = strstr(lineBuf, token);
    if (pValue != NULL)
    {
        *keyToSet = KeyCodeFromConfigName(pValue + strlen(token) - 1);
        return true;
    }
    return false;
}

static bool TryGetBool(const char* lineBuf, const char* token, bool* boolToSet)
{
    const char* pValue = strstr(lineBuf, token);
    if (pValue != NULL)
    {
        if (strstr(pValue + strlen(token) - 1, "true") != NULL)
        {
            *boolToSet = true;
            return true;
        }
        else if (strstr(pValue + strlen(token) - 1, "false") != NULL)
        {
            *boolToSet = false;
            return true;
        }
    }
    return false;
}

static bool TryGetFloat(const char* lineBuf, const char* token, float* floatToSet)
{
    const char* pValue = strstr(lineBuf, token);
    if (pValue != NULL)
    {
        *floatToSet = strtof(pValue + strlen(token)  - 1, NULL);
        return true;
    }
    return false;
}

static bool TryGetEnum(const char* lineBuf, const char* token,
    unsigned int* outValue, const char** enumStrings, unsigned int enumCount)
{
    const char* pValue = strstr(lineBuf, token);
    if (pValue == NULL)
        return false;
    for (unsigned int i = 0; i < enumCount; i++)
    {
        if (strstr(pValue + strlen(token) - 1, enumStrings[i]) != NULL)
        {
            *outValue = i;
            return true;
        }
    }
    return false;
}

void LoadConfig(Config* config)
{
    config->_Key._AppHold = VK_LMENU;
    config->_Key._AppSwitch = VK_TAB;
    config->_Key._WinHold = VK_LMENU;
    config->_Key._WinSwitch = VK_OEM_3;
    config->_Key._Invert = VK_LSHIFT;
    config->_Key._PrevApp = 0xFFFFFFFF;
    config->_Mouse = true;
    config->_CheckForUpdates = true;
    config->_ThemeMode = ThemeModeAuto;
    config->_AppSwitcherMode = AppSwitcherModeApp;
    config->_Scale = 1.5;

    const char* configFile = "AltAppSwitcherConfig.txt";
    FILE* file = fopen(configFile ,"rb");
    if (file == NULL)
    {
        file = fopen(configFile ,"a");
        fprintf(file, ConfigStr);
        fclose(file);
        fopen(configFile ,"rb");
    }

    static const char* themeModeStrings[] =
    {
        "auto",
        "light",
        "dark"
    };

    static const char* appSwitcherModeStrings[] =
    {
        "app",
        "window"
    };

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

    static char lineBuf[1024];
    while (fgets(lineBuf, 1024, file))
    {
        if (!strncmp(lineBuf, "//", 2))
            continue;
        if (TryGetKey(lineBuf, "app hold key: ", &config->_Key._AppHold))
            continue;
        if (TryGetKey(lineBuf, "next app key: ", &config->_Key._AppSwitch))
            continue;
        if (TryGetKey(lineBuf, "previous app key: ", &config->_Key._PrevApp))
            continue;
        if (TryGetKey(lineBuf, "window hold key: ", &config->_Key._WinHold))
            continue;
        if (TryGetKey(lineBuf, "next window key: ", &config->_Key._WinSwitch))
            continue;
        if (TryGetKey(lineBuf, "invert order key: ", &config->_Key._Invert))
            continue;
        if (TryGetBool(lineBuf, "allow mouse: ", &config->_Mouse))
            continue;
        if (TryGetEnum(lineBuf, "theme: ", &config->_ThemeMode, themeModeStrings, ARRAY_SIZE(themeModeStrings)))
            continue;
        if (TryGetEnum(lineBuf, "theme: ", &config->_AppSwitcherMode, appSwitcherModeStrings, ARRAY_SIZE(appSwitcherModeStrings)))
            continue;
        if (TryGetFloat(lineBuf, "scale: ", &config->_Scale))
            continue;
        if (TryGetBool(lineBuf, "check for updates: ", &config->_CheckForUpdates))
            continue;
    }
    fclose(file);

#undef ARRAY_SIZE
}