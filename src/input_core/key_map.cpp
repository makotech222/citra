// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "input_core/key_map.h"

namespace KeyMap {
const std::array<Service::HID::PadState, Settings::NativeInput::NUM_INPUTS> mapping_targets = {{
    Service::HID::PAD_A,           Service::HID::PAD_B,
    Service::HID::PAD_X,           Service::HID::PAD_Y,
    Service::HID::PAD_L,           Service::HID::PAD_R,
    Service::HID::PAD_ZL,          Service::HID::PAD_ZR,
    Service::HID::PAD_START,       Service::HID::PAD_SELECT,
    Service::HID::PAD_TOUCH,       Service::HID::PAD_UP,
    Service::HID::PAD_DOWN,        Service::HID::PAD_LEFT,
    Service::HID::PAD_RIGHT,       Service::HID::PAD_C_UP,
    Service::HID::PAD_C_DOWN,      Service::HID::PAD_C_LEFT,
    Service::HID::PAD_C_RIGHT,

    Service::HID::PAD_CIRCLE_UP,   Service::HID::PAD_CIRCLE_DOWN,
    Service::HID::PAD_CIRCLE_LEFT, Service::HID::PAD_CIRCLE_RIGHT,
}};
/// Array of inputs that are analog only, and require a strength when set
const std::array<Service::HID::PadState, 4> analog_inputs = {
    Service::HID::PAD_CIRCLE_UP, Service::HID::PAD_CIRCLE_DOWN, Service::HID::PAD_CIRCLE_LEFT,
    Service::HID::PAD_CIRCLE_RIGHT};
}
