// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <cmath>
#include <memory>
#include <SDL.h>
#include <SDL_Joystick.h>
#include "common/assert.h"
#include "common/logging/log.h"
#include "common/string_util.h"
#include "sdl_joystick.h"

bool SDLJoystick::SDLInitialized = false;

SDLJoystick::SDLJoystick() {}

SDLJoystick::SDLJoystick(int number_, _SDL_Joystick* gamepad_)
    : number(number_), gamepad(gamepad_) {}

SDLJoystick::~SDLJoystick() {
    CloseDevice();
}

bool SDLJoystick::InitDevice(int number_) {
    if (!SDLJoystick::SDLInitialized && SDL_Init(SDL_INIT_JOYSTICK) < 0) {
        LOG_CRITICAL(Input, "SDL_Init(SDL_INIT_JOYSTICK) failed");
        return false;
    }
    SDLJoystick::SDLInitialized = true;

    // Check for joystick
    if (SDL_NumJoysticks() > number_) {
        // Open joystick
        gamepad = SDL_JoystickOpen(number_);

        if (!gamepad) {
            LOG_INFO(Input, "Couldn't open Joystick %d", number_);
            return false;
        }
    }
    number = number_;
    GatherInputs();
    return true;
}

std::map<Settings::InputDeviceMapping, float> SDLJoystick::ProcessInput() {
    std::map<Settings::InputDeviceMapping, float> button_status;
    if (gamepad == nullptr)
        return button_status;
    SDL_JoystickUpdate();
    int i = 0;
    for (const auto& input : inputs) {
        auto strength = input->GetState();
        auto input_device_mapping =
            Settings::InputDeviceMapping(GetInputDeviceMapping() + input->GetName());
        button_status.emplace(input_device_mapping, strength);
    }
    return button_status;
}

bool SDLJoystick::CloseDevice() {
    if (gamepad != nullptr) {
        SDL_JoystickClose(gamepad);
    }
    return true;
}

void SDLJoystick::Clear() {}

std::vector<std::shared_ptr<InputDeviceInterface>> SDLJoystick::GetAllDevices() {
    if (!SDLJoystick::SDLInitialized && SDL_Init(SDL_INIT_JOYSTICK) < 0) {
        LOG_CRITICAL(Input, "SDL_Init(SDL_INIT_JOYSTICK) failed");
        return std::vector<std::shared_ptr<InputDeviceInterface>>();
    }
    SDLJoystick::SDLInitialized = true;
    std::vector<std::shared_ptr<InputDeviceInterface>> devices;
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        auto gamepad = std::make_shared<SDLJoystick>();
        bool success = gamepad->InitDevice(i);
        if (success)
            devices.push_back(gamepad);
    }
    return devices;
}

void SDLJoystick::GatherInputs() {
    for (size_t i = 0; i < SDL_JoystickNumButtons(gamepad); i++) {
        inputs.push_back(std::make_shared<Button>(i, gamepad));
    }
    for (size_t i = 0; i < SDL_JoystickNumHats(gamepad); i++) {
        for (size_t d = 0; d != 4; ++d)
            inputs.push_back(std::make_shared<Hat>(i, d, gamepad));
    }
    for (size_t i = 0; i < SDL_JoystickNumAxes(gamepad); i++) {
        inputs.push_back(std::make_shared<Axis>(i, 32767.0, gamepad));
        inputs.push_back(std::make_shared<Axis>(i, -32768.0, gamepad));
    }
}

float SDLJoystick::Button::GetState() {
    return SDL_JoystickGetButton(js, index);
}

std::string SDLJoystick::Button::GetName() {
    std::ostringstream ss;
    ss << "Button " << (int)index;
    return ss.str();
}

float SDLJoystick::Hat::GetState() {
    return (SDL_JoystickGetHat(js, index) & (1 << direction)) > 0;
}

std::string SDLJoystick::Hat::GetName() {
    char tmpstr[] = "Hat . .";
    tmpstr[4] = (char)('0' + index);
    tmpstr[6] = "NESW"[direction];
    return tmpstr;
}

float SDLJoystick::Axis::GetState() {
    double value = SDL_JoystickGetAxis(js, index);
    return std::max(0.0, value / range);
    ;
}

std::string SDLJoystick::Axis::GetName() {
    std::ostringstream ss;
    ss << "Axis " << (int)index << (range < 0 ? '-' : '+');
    return ss.str();
}
