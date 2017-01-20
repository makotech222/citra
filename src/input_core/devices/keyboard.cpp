// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <SDL_keyboard.h>
#include "input_core/devices/keyboard.h"

Keyboard::Keyboard() = default;

Keyboard::~Keyboard() = default;

bool Keyboard::InitDevice(int number) {
    return true;
}

std::map<Settings::InputDeviceMapping, float> Keyboard::ProcessInput() {
    std::map<KeyboardKey, bool> keys_pressed_copy;
    {
        std::lock_guard<std::mutex> lock(m);
        keys_pressed_copy = keys_pressed;
    }
    std::map<Settings::InputDeviceMapping, float> button_status;
    for (const auto& key : keys_pressed_copy) {
        auto input_device_mapping =
            Settings::InputDeviceMapping(GetInputDeviceMapping() + std::to_string(key.first.key));
        button_status.emplace(input_device_mapping, key.second ? 1.0 : 0.0);
    }
    return button_status;
}

bool Keyboard::CloseDevice() {
    return true;
}

void Keyboard::KeyPressed(KeyboardKey key) {
    std::lock_guard<std::mutex> lock(m);
    keys_pressed[key] = true;
}

void Keyboard::KeyReleased(KeyboardKey key) {
    std::lock_guard<std::mutex> lock(m);
    keys_pressed[key] = false;
}

void Keyboard::Clear() {
    std::lock_guard<std::mutex> lock(m);
    keys_pressed.clear();
}
