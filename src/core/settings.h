// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <map>
#include <memory>
#include <string>
#include <tuple>

#include "common/common_types.h"
#include "common/string_util.h"

namespace Settings {

enum class LayoutOption {
    Default,
    SingleScreen,
    LargeScreen,
    Custom,
};

namespace NativeInput {

enum Values {
    // directly mapped keys
    A,
    B,
    X,
    Y,
    L,
    R,
    ZL,
    ZR,
    START,
    SELECT,
    HOME,
    DUP,
    DDOWN,
    DLEFT,
    DRIGHT,
    CUP,
    CDOWN,
    CLEFT,
    CRIGHT,

    // indirectly mapped keys
    CIRCLE_UP,
    CIRCLE_DOWN,
    CIRCLE_LEFT,
    CIRCLE_RIGHT,

    NUM_INPUTS
};
static const std::array<const char*, NUM_INPUTS> Mapping = {
    {// directly mapped keys
     "pad_a", "pad_b", "pad_x", "pad_y", "pad_l", "pad_r", "pad_zl", "pad_zr", "pad_start",
     "pad_select", "pad_home", "pad_dup", "pad_ddown", "pad_dleft", "pad_dright", "pad_cup",
     "pad_cdown", "pad_cleft", "pad_cright",

     // indirectly mapped keys
     "pad_circle_up", "pad_circle_down", "pad_circle_left", "pad_circle_right"}};
static const std::array<Values, NUM_INPUTS> All = {
    {A,     B,      X,      Y,         L,           R,           ZL,          ZR,
     START, SELECT, HOME,   DUP,       DDOWN,       DLEFT,       DRIGHT,      CUP,
     CDOWN, CLEFT,  CRIGHT, CIRCLE_UP, CIRCLE_DOWN, CIRCLE_LEFT, CIRCLE_RIGHT}};
}

enum class DeviceFramework { SDL };
enum class Device { Keyboard, Gamepad };
static const std::map<std::string, DeviceFramework> device_framework_mapping = {
    {"SDL", DeviceFramework::SDL},
};
static const std::map<std::string, Device> device_mapping = {
    {"Keyboard", Device::Keyboard}, {"Gamepad", Device::Gamepad},
};
struct InputDeviceMapping {
    DeviceFramework framework = DeviceFramework::SDL;
    int number = 0;
    Device device = Device::Keyboard;
    std::string key;

    InputDeviceMapping() = default;
    explicit InputDeviceMapping(const std::string& input) {
        std::vector<std::string> parts;
        Common::SplitString(input, '/', parts);
        if (parts.size() != 4)
            return;

        if (device_framework_mapping.find(parts[0]) == device_framework_mapping.end())
            return;
        else
            framework = device_framework_mapping.at(parts[0]);

        number = std::stoi(parts[1]);

        if (device_mapping.find(parts[2]) == device_mapping.end())
            return;
        else
            device = device_mapping.at(parts[2]);

        key = parts[3];
    }

    bool operator==(const InputDeviceMapping& rhs) const {
        return std::tie(device, framework, number) ==
               std::tie(rhs.device, rhs.framework, rhs.number);
    }
    bool operator==(const std::string& rhs) const {
        return ToString() == rhs;
    }
    bool operator<(const InputDeviceMapping& rhs) const {
        return ToString() < rhs.ToString();
    }

    void InitKey() {}

    std::string ToString() const {
        std::string result;
        result = (std::find_if(device_framework_mapping.begin(), device_framework_mapping.end(),
                               [&](std::pair<std::string, DeviceFramework> x) {
                                   return x.second == framework;
                               }))
                     ->first;

        result += "/";
        result += std::to_string(number);
        result += "/";

        result +=
            (std::find_if(device_mapping.begin(), device_mapping.end(),
                          [&](std::pair<std::string, Device> x) { return x.second == device; }))
                ->first;

        result += "/";
        result += key;
        return result;
    }
};

struct Values {
    // CheckNew3DS
    bool is_new_3ds;

    // Controls
    std::array<InputDeviceMapping, NativeInput::NUM_INPUTS> input_mappings;
    InputDeviceMapping pad_circle_modifier;
    float pad_circle_modifier_scale;
    float pad_circle_deadzone;

    // Core
    bool use_cpu_jit;

    // Data Storage
    bool use_virtual_sd;

    // System Region
    int region_value;

    // Renderer
    bool use_hw_renderer;
    bool use_shader_jit;
    float resolution_factor;
    bool use_vsync;
    bool toggle_framelimit;

    LayoutOption layout_option;
    bool swap_screen;

    float bg_red;
    float bg_green;
    float bg_blue;

    std::string log_filter;

    // Audio
    std::string sink_id;
    bool enable_audio_stretching;

    // Debugging
    bool use_gdbstub;
    u16 gdbstub_port;
};
extern Values values;

// a special value for Values::region_value indicating that citra will automatically select a region
// value to fit the region lockout info of the game
static constexpr int REGION_VALUE_AUTO_SELECT = -1;

void Apply();
}
