// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <tuple>

#include "core/hle/service/hid/hid.h"
#include "core/settings.h"
#include "input_core/devices/device.h"

class Keyboard;

class InputCore {
public:
    static void Init();
    static void Shutdown();

    /**
     * Threadsafe getter to the current PadState
     * @return Service::HID::PadState instance
     */
    static Service::HID::PadState GetPadState();

    /**
     * Threadsafe setter for the current PadState
     * @param state New PadState to overwrite current PadState.
     */
    static void SetPadState(const Service::HID::PadState& state);

    /**
     * Getter for current CirclePad
     * @return std::tuple<s16, s16> CirclePad state
     */
    static std::tuple<s16, s16> GetCirclePad();

    /**
     * Getter for Citra's main keyboard input handler
     * @return std::shared_ptr<Keyboard> Device Keyboard instance
     */
    static std::shared_ptr<Keyboard> GetKeyboard();

    /**
     * Gets the current touch screen state (touch X/Y coordinates and whether or not it is pressed).
     * Threadsafe.
     * @note This should be called by the core emu thread to get a state set by the window thread.
     * @return std::tuple of (x, y, pressed) where `x` and `y` are the touch coordinates and
     *         `pressed` is true if the touch screen is currently being pressed
     */
    static std::tuple<u16, u16, bool> GetTouchState();

    /**
     * Threadsafe setter for the current touch screen state.
     * @param value New Touch State
     */
    static void SetTouchState(std::tuple<u16, u16, bool> value);

    /**
     * Reload input key mapping settings during game-play
     */
    static void ReloadSettings();
    /**
     * Returns vector of all available devices from user's system.
     */
    static std::vector<std::shared_ptr<IDevice>> GetAllDevices();

    /**
     * Loops through all devices and detects the first device that produces an input
     * @param max_time: maximum amount of time to wait until input detected, in milliseconds.
     * @param update_GUI: function to run in while loop to process any gui events.
     * @return Settings::InputDeviceMapping of input device
     */
    static Settings::InputDeviceMapping DetectInput(int max_time,
                                                    std::function<void(void)> update_GUI);

private:
    static int tick_event;
    static Service::HID::PadState pad_state;
    static std::tuple<s16, s16> circle_pad;
    static std::shared_ptr<Keyboard> main_keyboard; ///< Keyboard is always active for Citra
    static std::vector<std::shared_ptr<IDevice>>
        devices; ///< Devices that are handling input for the game
    static std::map<Settings::InputDeviceMapping, std::vector<Service::HID::PadState>> key_mappings;
    static std::map<Service::HID::PadState, bool>
        keys_pressed; ///< keys that were pressed on previous frame.
    static std::mutex pad_state_mutex;
    static std::mutex touch_mutex;
    static u16 touch_x;        ///< Touchpad X-position in native 3DS pixel coordinates (0-320)
    static u16 touch_y;        ///< Touchpad Y-position in native 3DS pixel coordinates (0-240)
    static bool touch_pressed; ///< True if touchpad area is currently pressed, otherwise false
    static const float input_detect_threshold;

    /**
     * Loops through all unique input devices, and all bound inputs to update the emulator's input
     * status.
     */
    static void UpdateEmulatorInputs(
        std::vector<std::map<Settings::InputDeviceMapping, float>> inputs);

    static void InputTickCallback(u64, int cycles_late);

    /**
     * Helper methodto check if device was already initialized
     */
    static bool CheckIfMappingExists(const std::vector<Settings::InputDeviceMapping>& uniqueMapping,
                                     Settings::InputDeviceMapping mappingToCheck);

    static std::vector<Settings::InputDeviceMapping>
    GatherUniqueMappings(); /// Get unique input mappings from settings

    static void BuildKeyMapping(); /// Builds map of input keys to 3ds buttons for unique device

    static void GenerateUniqueDevices(); /// Generate a device for each unique mapping

    static void ParseSettings(); /// Read settings to initialize devices

    /**
     * Takes two floats and the deadzone and applies formula to
     * correct the stick position.
     */
    static std::tuple<float, float> ApplyDeadzone(float x, float y, float dead_zone);
};
