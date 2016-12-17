// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <SDL_keyboard.h>

#include "input_core/devices/keyboard.h"

Keyboard::Keyboard() {}

Keyboard::~Keyboard() {}

bool Keyboard::InitDevice(int number, Settings::InputDeviceMapping device_mapping) {
    input_device_mapping = device_mapping;
    return true;
}

std::map<Settings::InputDeviceMapping, float> Keyboard::ProcessInput() {
    std::map<KeyboardKey, bool> keysPressedCopy;
    {
        std::lock_guard<std::mutex> lock(m);
        keysPressedCopy = keys_pressed;
    }
    std::map<Settings::InputDeviceMapping, float> button_status;
    bool circlePadModPressed = keysPressedCopy[circle_pad_modifier];
    for (const auto& key : keysPressedCopy) {
        input_device_mapping.key = key.first.key;
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

Settings::InputDeviceMapping Keyboard::GetInput() {
    auto result = ProcessInput();
    for (const auto& entry : result) {
        if (entry.second > 0.5)
            return entry.first;
    }
    return Settings::InputDeviceMapping("");
}
