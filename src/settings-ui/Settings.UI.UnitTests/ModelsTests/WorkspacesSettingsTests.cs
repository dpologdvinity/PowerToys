// Copyright (c) Microsoft Corporation
// The Microsoft Corporation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using Microsoft.PowerToys.Settings.UI.Library;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace CommonLibTest
{
    [TestClass]
    public class WorkspacesSettingsTests
    {
        // The key below Esc on the current keyboard layout, which the default shortcut binds to.
        private static readonly int DefaultHotkeyKey = WorkspacesProperties.GetDefaultHotkeyKeyCode();

        [TestMethod]
        public void Defaults_ShouldBindWinCtrlToKeyBelowEscape()
        {
            var settings = new WorkspacesSettings();

            // Win+Ctrl+` on US layouts.
            AssertHotkey(settings.Properties.Hotkey.Value, win: true, ctrl: true, alt: false, shift: false, code: DefaultHotkeyKey);
        }

        [TestMethod]
        public void HotkeyAccessor_WithNull_ShouldResetToDefault()
        {
            var settings = new WorkspacesSettings();
            settings.Properties.Hotkey.Value = new HotkeySettings(true, false, false, false, 0x4E); // Win+N

            settings.GetAllHotkeyAccessors()[0].Value = null;

            AssertHotkey(settings.Properties.Hotkey.Value, win: true, ctrl: true, alt: false, shift: false, code: DefaultHotkeyKey);
        }

        [TestMethod]
        [DataRow(0xC0u, 0xC0, DisplayName = "US: VK_OEM_3")]
        [DataRow(0xDFu, 0xDF, DisplayName = "UK: VK_OEM_8")]
        [DataRow(0xDCu, 0xDC, DisplayName = "German: VK_OEM_5")]
        [DataRow(0xDEu, 0xDE, DisplayName = "French: VK_OEM_7")]
        [DataRow(0xF3u, 0xC0, DisplayName = "Japanese: half-width/full-width IME key falls back to VK_OEM_3")]
        [DataRow(0xF4u, 0xC0, DisplayName = "Japanese: half-width/full-width IME key (alternate VK) falls back to VK_OEM_3")]
        [DataRow(0u, 0xC0, DisplayName = "Unmapped: falls back to VK_OEM_3")]
        [DataRow(0x100u, 0xC0, DisplayName = "Out of range: falls back to VK_OEM_3")]
        public void GetDefaultHotkeyKeyCode_ShouldUseLayoutKeyBelowEscape(uint layoutVirtualKey, int expected)
        {
            Assert.AreEqual(expected, WorkspacesProperties.GetDefaultHotkeyKeyCode(layoutVirtualKey));
        }

        private static void AssertHotkey(HotkeySettings hotkey, bool win, bool ctrl, bool alt, bool shift, int code)
        {
            Assert.IsNotNull(hotkey);
            Assert.AreEqual(win, hotkey.Win, "Win modifier mismatch.");
            Assert.AreEqual(ctrl, hotkey.Ctrl, "Ctrl modifier mismatch.");
            Assert.AreEqual(alt, hotkey.Alt, "Alt modifier mismatch.");
            Assert.AreEqual(shift, hotkey.Shift, "Shift modifier mismatch.");
            Assert.AreEqual(code, hotkey.Code, "Key code mismatch.");
        }
    }
}
