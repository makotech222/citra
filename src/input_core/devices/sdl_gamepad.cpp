// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cmath>
#include <memory>
#include <SDL.h>
#include <SDL_gamecontroller.h>

#include "common/assert.h"
#include "common/logging/log.h"
#include "common/string_util.h"

#include "input_core/devices/gamecontrollerdb.h"
#include "input_core/devices/sdl_gamepad.h"

bool SDLGamepad::SDLInitialized = false;

SDLGamepad::SDLGamepad() {}
SDLGamepad::SDLGamepad(int number_, _SDL_GameController* gamepad_)
    : number(number_), gamepad(gamepad_) {}
SDLGamepad::~SDLGamepad() {
    CloseDevice();
}

bool SDLGamepad::InitDevice(int number, Settings::InputDeviceMapping device_mapping) {
    if (!SDLGamepad::SDLInitialized && SDL_Init(SDL_INIT_GAMECONTROLLER) < 0) {
        LOG_CRITICAL(Input, "SDL_Init(SDL_INIT_GAMECONTROLLER) failed");
        return false;
    }
    SDL_GameControllerEventState(SDL_IGNORE);
    SDLGamepad::SDLInitialized = true;
    LoadGameControllerDB();

    if (SDL_IsGameController(number)) {
        gamepad = SDL_GameControllerOpen(number);
        if (gamepad == nullptr) {
            return false;
        }
    }
    input_device_mapping = device_mapping;
    return true;
}

std::map<Settings::InputDeviceMapping, float> SDLGamepad::ProcessInput() {
    std::map<Settings::InputDeviceMapping, float> button_status;
    if (gamepad == nullptr)
        return button_status;
    SDL_GameControllerUpdate();
    for (int i = 0; i < SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_MAX; i++) {
        SDL_GameControllerButton button = static_cast<SDL_GameControllerButton>(i);
        Uint8 pressed = SDL_GameControllerGetButton(gamepad, button);
        input_device_mapping.key = static_cast<int>(
            gamepadinput_to_sdlname_mapping2[SDL_GameControllerGetStringForButton(button)]);
        button_status.emplace(input_device_mapping, pressed);
    }
    for (int i = 0; i < SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_MAX; i++) {
        SDL_GameControllerAxis axis = static_cast<SDL_GameControllerAxis>(i);
        float strength = fmaxf(-1, (float)SDL_GameControllerGetAxis(gamepad, axis) / 32767.0);
        input_device_mapping.key = static_cast<int>(
            gamepadinput_to_sdlname_mapping2[SDL_GameControllerGetStringForAxis(axis)]);
        if (strength < 0) {
            button_status.emplace(input_device_mapping, 0);
            input_device_mapping.key += 1; // minus axis value is always one greater
            button_status.emplace(input_device_mapping, abs(strength));
        } else if (strength >= 0 && i < 4) {
            button_status.emplace(input_device_mapping, abs(strength));
            input_device_mapping.key += 1;
            button_status.emplace(input_device_mapping, 0);
        } else { // Only trigger buttons
            button_status.emplace(input_device_mapping, abs(strength));
        }
    }
    return button_status;
}

bool SDLGamepad::CloseDevice() {
    if (gamepad != nullptr) {
        SDL_GameControllerClose(gamepad);
    }
    return true;
}

std::vector<std::shared_ptr<IDevice>> SDLGamepad::GetAllDevices() {
    std::vector<std::shared_ptr<IDevice>> devices;
    for (int i = 0; i < 8; i++) {
        auto gamepad = std::make_shared<SDLGamepad>();
        bool success = gamepad->InitDevice(
            i, Settings::InputDeviceMapping("SDL/" + std::to_string(i) + "/Gamepad/-1"));
        if (success)
            devices.push_back(gamepad);
    }
    return devices;
}

void SDLGamepad::LoadGameControllerDB() {
    std::vector<std::string> lines1, lines2, lines3, lines4;
    Common::SplitString(SDLGameControllerDB::db_file1, '\n', lines1);
    Common::SplitString(SDLGameControllerDB::db_file2, '\n', lines2);
    Common::SplitString(SDLGameControllerDB::db_file3, '\n', lines3);
    Common::SplitString(SDLGameControllerDB::db_file4, '\n', lines4);
    lines1.insert(lines1.end(), lines2.begin(), lines2.end());
    lines1.insert(lines1.end(), lines3.begin(), lines3.end());
    lines1.insert(lines1.end(), lines4.begin(), lines4.end());
    for (std::string s : lines1) {
        SDL_GameControllerAddMapping(s.c_str());
    }
}

Settings::InputDeviceMapping SDLGamepad::GetInput() {
    if (gamepad == nullptr)
        return Settings::InputDeviceMapping("");

    auto results = ProcessInput();
    for (auto& input : results) {
        if (input.second > 0.5)
            return input.first;
    }
    return Settings::InputDeviceMapping("");
}

void SDLGamepad::Clear() {}
