#pragma once

#include <windows.h>

inline constexpr wchar_t PERSONALIZATION_REGISTRY_PATH[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
inline constexpr wchar_t NIGHT_LIGHT_REGISTRY_PATH[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\CloudStore\\Store\\DefaultAccount\\Current\\default$windows.data.bluelightreduction.bluelightreductionstate\\windows.data.bluelightreduction.bluelightreductionstate";
inline constexpr wchar_t LIGHT_SWITCH_TOGGLE_REQUEST_SEMAPHORE[] = L"Local\\PowerToys-LightSwitch-ToggleRequest-49904";
// PowerToys-owned state that remembers "Show accent color on Start and taskbar" across a light theme period.
inline constexpr wchar_t LIGHT_SWITCH_STATE_REGISTRY_PATH[] = L"Software\\Microsoft\\PowerToys\\LightSwitch";
inline constexpr wchar_t RESTORE_COLOR_PREVALENCE_VALUE_NAME[] = L"RestoreColorPrevalence";

// Reads report registry failures without guessing a theme.
LSTATUS TryGetSystemTheme(bool& isLight);
LSTATUS TryGetAppsTheme(bool& isLight);
// Writes report registry errors and broadcast changes. The service verifies the final theme snapshot.
LSTATUS TrySetSystemTheme(bool isLight);
LSTATUS TrySetAppsTheme(bool isLight);
// Leaves enabled unchanged when the registry value cannot be read or validated.
LSTATUS TryGetNightLightState(bool& enabled);

namespace LightSwitchThemeHelpers
{
    // The caller owns an open personalization key. These helpers do not broadcast.
    LSTATUS ReadThemeValue(HKEY key, const wchar_t* name, bool& isLight);
    LSTATUS WriteThemeValue(HKEY key, const wchar_t* name, bool isLight);

    // The caller owns an open Night Light key. Failure leaves enabled unchanged.
    LSTATUS ReadNightLightState(HKEY key, bool& enabled);

    // Windows turns off the Start and taskbar accent color in light mode and does not restore it in
    // dark mode. The caller owns open personalization and state keys with query and set access.
    // wasLight is the system theme before the switch. These helpers do not broadcast.
    // Light: remembers an enabled accent color, then turns it off. A null stateKey only turns it off.
    LSTATUS SaveAndResetColorPrevalence(HKEY personalizationKey, HKEY stateKey, bool wasLight);
    // Dark: turns a remembered accent color back on after a light period and forgets it.
    LSTATUS RestoreColorPrevalence(HKEY personalizationKey, HKEY stateKey, bool wasLight, bool& restored);
}

void SetSystemTheme(bool isLight);
void SetAppsTheme(bool isLight);
bool GetCurrentSystemTheme();
bool GetCurrentAppsTheme();
