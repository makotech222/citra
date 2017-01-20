// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "input_core/devices/device.h"

struct _SDL_Joystick;
class SDLJoystick : public InputDeviceInterface {
public:
    SDLJoystick();
    SDLJoystick(int number_, _SDL_Joystick* gamepad_);
    ~SDLJoystick();

    bool InitDevice(int number) override;
    std::map<Settings::InputDeviceMapping, float> ProcessInput() override;
    bool CloseDevice() override;
    void Clear() override;

    /// Returns vector of all gamepads connected to computer. Used for keybinding setup
    static std::vector<std::shared_ptr<InputDeviceInterface>> GetAllDevices();

protected:
    std::string GetInputDeviceMapping() override {
        return "SDL/" + std::to_string(number) + "/Gamepad/";
    }

private:
    static bool SDLInitialized;
    _SDL_Joystick* gamepad = nullptr;
    int number; ///< Index of gamepad connection
    float input_detect_threshold = 0.8;
    std::vector<std::shared_ptr<Input>> inputs;

    void GatherInputs();

    class Button : public Input {
    public:
        float GetState() override;
        std::string GetName() override;
        Button(u8 index_, _SDL_Joystick* js_) : index(index_), js(js_) {}

    private:
        const u8 index;
        _SDL_Joystick* js;
    };
    class Hat : public Input {
    public:
        float GetState() override;
        std::string GetName() override;
        Hat(u8 index_, u8 direction_, _SDL_Joystick* js_)
            : direction(direction_), index(index_), js(js_) {}

    private:
        const u8 index;
        const u8 direction;
        _SDL_Joystick* js;
    };
    class Axis : public Input {
    public:
        float GetState() override;
        std::string GetName() override;
        Axis(u8 index_, s16 range_, _SDL_Joystick* js_) : range(range_), index(index_), js(js_) {}

    private:
        const u8 index;
        const s16 range;
        _SDL_Joystick* js;
    };
};
