// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>

#include "core/hle/service/hid/hid.h"

namespace KeyMap {
extern const std::array<Service::HID::PadState, Settings::NativeInput::NUM_INPUTS> mapping_targets;
extern const std::array<Service::HID::PadState, 4> analog_inputs;
constexpr int MAX_CIRCLEPAD_POS = 0x9C; /// Max value for a circle pad position
}
