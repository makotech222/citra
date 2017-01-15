// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "input_core/devices/device.h"

struct _SDL_GameController;
class SDLGamepad : public InputDeviceInterface {
public:
    SDLGamepad();
    SDLGamepad(int number_, _SDL_GameController* gamepad_);
    ~SDLGamepad();

    bool InitDevice(int number, Settings::InputDeviceMapping device_mapping) override;
    std::map<Settings::InputDeviceMapping, float> ProcessInput() override;
    bool CloseDevice() override;
    Settings::InputDeviceMapping GetInput() override;
    void Clear() override;

    /// Returns vector of all gamepads connected to computer. Used for keybinding setup
    static std::vector<std::shared_ptr<InputDeviceInterface>> GetAllDevices();
    enum class GamepadInputs {
        ButtonA,
        ButtonB,
        ButtonX,
        ButtonY,
        LeftShoulder,
        RightShoulder,
        Start,
        Back,
        DPadUp,
        DpadDown,
        DpadLeft,
        DpadRight,
        L3,
        R3,
        LeftTrigger,
        RightTrigger,
        LeftYPlus,
        LeftYMinus,
        LeftXPlus,
        LeftXMinus,
        RightYPlus,
        RightYMinus,
        RightXPlus,
        RightXMinus,
        MAX
    };

private:
    /// Maps the friendly name shown on GUI with the string name for getting the SDL button
    /// instance.
    std::map<std::string, GamepadInputs> gamepadinput_to_sdlname_mapping = {
        {"a", GamepadInputs::ButtonA},
        {"b", GamepadInputs::ButtonB},
        {"x", GamepadInputs::ButtonX},
        {"y", GamepadInputs::ButtonY},
        {"leftshoulder", GamepadInputs::LeftShoulder},
        {"rightshoulder", GamepadInputs::RightShoulder},
        {"start", GamepadInputs::Start},
        {"back", GamepadInputs::Back},
        {"dpup", GamepadInputs::DPadUp},
        {"dpdown", GamepadInputs::DpadDown},
        {"dpleft", GamepadInputs::DpadLeft},
        {"dpright", GamepadInputs::DpadRight},
        {"leftstick", GamepadInputs::L3},
        {"rightstick", GamepadInputs::R3},
        {"lefttrigger", GamepadInputs::LeftTrigger},
        {"righttrigger", GamepadInputs::RightTrigger},
        {"lefty", GamepadInputs::LeftYPlus},
        {"lefty2", GamepadInputs::LeftYMinus},
        {"leftx", GamepadInputs::LeftXPlus},
        {"leftx2", GamepadInputs::LeftXMinus},
        {"righty", GamepadInputs::RightYPlus},
        {"righty2", GamepadInputs::RightYMinus},
        {"rightx", GamepadInputs::RightXPlus},
        {"rightx2", GamepadInputs::RightXMinus}};
    static bool SDLInitialized;
    std::map<std::string, bool>
        keys_pressed; ///< Map of keys that were pressed on previous iteration
    _SDL_GameController* gamepad = nullptr;
    int number; ///< Index of gamepad connection
    float input_detect_threshold = 0.8;

    static void LoadGameControllerDB();
};
