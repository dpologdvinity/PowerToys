// Copyright (c) Microsoft Corporation
// The Microsoft Corporation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System.Text.Json;
using System.Text.Json.Serialization;

using Microsoft.PowerToys.Settings.UI.Library.Utilities;

namespace Microsoft.PowerToys.Settings.UI.Library
{
    public class WorkspacesProperties
    {
        public enum SortByProperty
        {
            LastLaunched,
            Created,
            Name,
        }

        // Scan code of the physical key below Esc (` ~ on US layouts). The default shortcut
        // binds to this key rather than to a fixed virtual-key code, because its VK differs by
        // layout: VK_OEM_3 on US, but VK_OEM_8 on UK where VK_OEM_3 is ' @.
        private const uint DefaultHotkeyScanCode = 0x29;

        // VK_OEM_3
        private const int FallbackHotkeyKeyCode = 0xC0;

        // VK_OEM_1 through VK_OEM_102, the layout-dependent character keys.
        private const uint FirstOemKeyCode = 0xBA;
        private const uint LastOemKeyCode = 0xE2;

        public WorkspacesProperties()
        {
            Hotkey = new KeyboardKeysProperty(GetDefaultHotkeyValue());
        }

        [JsonPropertyName("hotkey")]
        public KeyboardKeysProperty Hotkey { get; set; }

        [JsonPropertyName("sortby")]
        public SortByProperty SortBy { get; set; }

        // Win+Ctrl+` on US layouts.
        public static HotkeySettings GetDefaultHotkeyValue()
        {
            return new HotkeySettings(true, true, false, false, GetDefaultHotkeyKeyCode());
        }

        // The VK of the key below Esc on the current keyboard layout.
        public static int GetDefaultHotkeyKeyCode()
        {
            return GetDefaultHotkeyKeyCode(NativeMethods.MapVirtualKeyEx(DefaultHotkeyScanCode, NativeMethods.MAPVK_VSC_TO_VK, NativeMethods.GetKeyboardLayout(0)));
        }

        // Falls back to VK_OEM_3 when the layout has no OEM character key below Esc, e.g. on
        // Japanese layouts, where that key is the half-width/full-width IME toggle (VK_OEM_AUTO).
        public static int GetDefaultHotkeyKeyCode(uint layoutVirtualKey)
        {
            return layoutVirtualKey is >= FirstOemKeyCode and <= LastOemKeyCode ? (int)layoutVirtualKey : FallbackHotkeyKeyCode;
        }

        public string ToJsonString()
        {
            return JsonSerializer.Serialize(this);
        }
    }
}
