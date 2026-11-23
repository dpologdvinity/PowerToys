#include "pch.h"
#include "ThemeHelper.h"
#include <logger/logger.h>
#include <memory>
#include <new>

namespace
{
    constexpr wchar_t COLOR_PREVALENCE_VALUE_NAME[] = L"ColorPrevalence";

    LSTATUS DeleteRememberedColorPrevalence(HKEY stateKey)
    {
        const auto result = RegDeleteValueW(stateKey, RESTORE_COLOR_PREVALENCE_VALUE_NAME);
        return result == ERROR_FILE_NOT_FOUND ? ERROR_SUCCESS : result;
    }
}

namespace LightSwitchThemeHelpers
{
    LSTATUS ReadThemeValue(HKEY key, const wchar_t* name, bool& isLight)
    {
        DWORD value = 0;
        DWORD size = sizeof(value);
        const auto result = RegGetValueW(key, nullptr, name, RRF_RT_REG_DWORD, nullptr, &value, &size);
        if (result != ERROR_SUCCESS)
        {
            return result;
        }
        if (size != sizeof(value) || value > 1)
        {
            return ERROR_INVALID_DATA;
        }
        isLight = value == 1;
        return ERROR_SUCCESS;
    }

    LSTATUS WriteThemeValue(HKEY key, const wchar_t* name, bool isLight)
    {
        const DWORD value = isLight ? 1 : 0;
        return RegSetValueExW(key, name, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&value), sizeof(value));
    }

    LSTATUS ReadNightLightState(HKEY key, bool& enabled)
    {
        constexpr DWORD minimumSize = 25;
        constexpr DWORD maximumSize = 64 * 1024;
        DWORD size = 0;
        auto result = RegGetValueW(key, nullptr, L"Data", RRF_RT_REG_BINARY, nullptr, nullptr, &size);
        if (result != ERROR_SUCCESS)
        {
            return result;
        }
        if (size < minimumSize)
        {
            return ERROR_INVALID_DATA;
        }
        if (size > maximumSize)
        {
            return ERROR_MORE_DATA;
        }

        // Bound the allocation and recheck the actual size: CloudStore can replace
        // this value between the size query and the data read.
        const std::unique_ptr<BYTE[]> data(new (std::nothrow) BYTE[size]);
        if (!data)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        result = RegGetValueW(key, nullptr, L"Data", RRF_RT_REG_BINARY, nullptr, data.get(), &size);
        if (result != ERROR_SUCCESS)
        {
            return result;
        }
        if (size < minimumSize)
        {
            return ERROR_INVALID_DATA;
        }

        enabled = data[23] == 0x10 && data[24] == 0x00;
        return ERROR_SUCCESS;
    }

    LSTATUS SaveAndResetColorPrevalence(HKEY personalizationKey, HKEY stateKey, bool wasLight)
    {
        bool enabled = false;
        ReadThemeValue(personalizationKey, COLOR_PREVALENCE_VALUE_NAME, enabled);

        LSTATUS stateResult = ERROR_SUCCESS;
        if (stateKey && enabled)
        {
            stateResult = WriteThemeValue(stateKey, RESTORE_COLOR_PREVALENCE_VALUE_NAME, true);
        }
        else if (stateKey && !wasLight)
        {
            // Leaving dark mode with the accent color off replaces any older remembered value.
            // Repeated light writes keep it, because the accent color is already off by then.
            stateResult = DeleteRememberedColorPrevalence(stateKey);
        }

        const auto result = WriteThemeValue(personalizationKey, COLOR_PREVALENCE_VALUE_NAME, false);
        return result != ERROR_SUCCESS ? result : stateResult;
    }

    LSTATUS RestoreColorPrevalence(HKEY personalizationKey, HKEY stateKey, bool wasLight, bool& restored)
    {
        restored = false;
        bool remembered = false;
        auto result = ReadThemeValue(stateKey, RESTORE_COLOR_PREVALENCE_VALUE_NAME, remembered);
        if (result == ERROR_FILE_NOT_FOUND)
        {
            return ERROR_SUCCESS;
        }

        // If the system was already dark, the user has had the chance to choose the accent
        // color since the light period, so only consume the remembered value.
        if (result == ERROR_SUCCESS && remembered && wasLight)
        {
            result = WriteThemeValue(personalizationKey, COLOR_PREVALENCE_VALUE_NAME, true);
            if (result != ERROR_SUCCESS)
            {
                return result;
            }
            restored = true;
        }

        return DeleteRememberedColorPrevalence(stateKey);
    }
}

namespace
{
    LSTATUS ReadPersonalizationTheme(const wchar_t* name, bool& isLight)
    {
        HKEY key = nullptr;
        auto result = RegOpenKeyExW(HKEY_CURRENT_USER, PERSONALIZATION_REGISTRY_PATH, 0, KEY_QUERY_VALUE, &key);
        if (result == ERROR_SUCCESS)
        {
            result = LightSwitchThemeHelpers::ReadThemeValue(key, name, isLight);
            RegCloseKey(key);
        }
        return result;
    }

    void BroadcastThemeChange()
    {
        SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, reinterpret_cast<LPARAM>(L"ImmersiveColorSet"), SMTO_ABORTIFHUNG, 5000, nullptr);
        SendMessageTimeoutW(HWND_BROADCAST, WM_THEMECHANGED, 0, 0, SMTO_ABORTIFHUNG, 5000, nullptr);
    }

    LSTATUS UpdateColorPrevalence(HKEY personalizationKey, bool isLight, bool wasLight, bool& restored)
    {
        restored = false;
        HKEY stateKey = nullptr;
        LSTATUS result = ERROR_SUCCESS;
        if (isLight)
        {
            const auto stateResult = RegCreateKeyExW(HKEY_CURRENT_USER, LIGHT_SWITCH_STATE_REGISTRY_PATH, 0, nullptr, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, nullptr, &stateKey, nullptr);
            if (stateResult != ERROR_SUCCESS)
            {
                stateKey = nullptr;
            }
            result = LightSwitchThemeHelpers::SaveAndResetColorPrevalence(personalizationKey, stateKey, wasLight);
            if (result == ERROR_SUCCESS)
            {
                result = stateResult;
            }
        }
        else
        {
            result = RegOpenKeyExW(HKEY_CURRENT_USER, LIGHT_SWITCH_STATE_REGISTRY_PATH, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &stateKey);
            if (result == ERROR_FILE_NOT_FOUND)
            {
                return ERROR_SUCCESS;
            }
            if (result != ERROR_SUCCESS)
            {
                return result;
            }
            result = LightSwitchThemeHelpers::RestoreColorPrevalence(personalizationKey, stateKey, wasLight, restored);
        }

        if (stateKey)
        {
            RegCloseKey(stateKey);
        }
        return result;
    }

    LSTATUS WritePersonalizationTheme(const wchar_t* name, bool isLight, bool system)
    {
        HKEY key = nullptr;
        auto result = RegOpenKeyExW(HKEY_CURRENT_USER, PERSONALIZATION_REGISTRY_PATH, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &key);
        if (result != ERROR_SUCCESS)
        {
            return result;
        }

        // An unreadable previous theme is treated as light, so a remembered accent color is kept.
        bool wasLight = true;
        if (system)
        {
            LightSwitchThemeHelpers::ReadThemeValue(key, name, wasLight);
        }

        bool restoredColorPrevalence = false;
        result = LightSwitchThemeHelpers::WriteThemeValue(key, name, isLight);
        if (result == ERROR_SUCCESS && system)
        {
            const auto colorResult = UpdateColorPrevalence(key, isLight, wasLight, restoredColorPrevalence);
            if (colorResult != ERROR_SUCCESS)
            {
                Logger::warn(L"[LightSwitchLib] Could not update ColorPrevalence (error: {}).", colorResult);
            }
        }
        RegCloseKey(key);

        if (result == ERROR_SUCCESS)
        {
            BroadcastThemeChange();
            if (system && (isLight || restoredColorPrevalence))
            {
                SendMessageTimeoutW(HWND_BROADCAST, WM_DWMCOLORIZATIONCOLORCHANGED, 0, 0, SMTO_ABORTIFHUNG, 5000, nullptr);
            }
        }
        return result;
    }
}

LSTATUS TryGetSystemTheme(bool& isLight)
{
    return ReadPersonalizationTheme(L"SystemUsesLightTheme", isLight);
}

LSTATUS TryGetAppsTheme(bool& isLight)
{
    return ReadPersonalizationTheme(L"AppsUseLightTheme", isLight);
}

LSTATUS TrySetSystemTheme(bool isLight)
{
    return WritePersonalizationTheme(L"SystemUsesLightTheme", isLight, true);
}

LSTATUS TrySetAppsTheme(bool isLight)
{
    return WritePersonalizationTheme(L"AppsUseLightTheme", isLight, false);
}

void SetSystemTheme(bool isLight)
{
    const auto result = TrySetSystemTheme(isLight);
    if (result != ERROR_SUCCESS)
    {
        Logger::warn(L"[LightSwitchLib] Could not set system theme (error: {}).", result);
    }
}

void SetAppsTheme(bool isLight)
{
    const auto result = TrySetAppsTheme(isLight);
    if (result != ERROR_SUCCESS)
    {
        Logger::warn(L"[LightSwitchLib] Could not set apps theme (error: {}).", result);
    }
}

bool GetCurrentSystemTheme()
{
    bool isLight = true;
    TryGetSystemTheme(isLight);
    return isLight;
}

bool GetCurrentAppsTheme()
{
    bool isLight = true;
    TryGetAppsTheme(isLight);
    return isLight;
}

LSTATUS TryGetNightLightState(bool& enabled)
{
    HKEY key = nullptr;
    auto result = RegOpenKeyExW(HKEY_CURRENT_USER, NIGHT_LIGHT_REGISTRY_PATH, 0, KEY_QUERY_VALUE, &key);
    if (result == ERROR_SUCCESS)
    {
        result = LightSwitchThemeHelpers::ReadNightLightState(key, enabled);
        RegCloseKey(key);
    }
    return result;
}
