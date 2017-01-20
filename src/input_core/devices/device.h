// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <map>
#include "core/hle/service/hid/hid.h"
#include "core/settings.h"
#include "input_core/key_map.h"

class InputDeviceInterface {
public:
    virtual ~InputDeviceInterface();

    /**
     * Initialize InputDeviceInterface object with device's index and the map of keys that it will
     * listen to.
     * @param number: device number as ordered connected to computer.
     * @param keymap: vector of PadStates for device to listen for
     * @return true if successful
     */
    virtual bool InitDevice(int number) = 0;

    /**
     * Process inputs that were pressed since last frame
     */
    virtual std::map<Settings::InputDeviceMapping, float> ProcessInput() = 0;

    /**
     * Close connection to device
     * @return true if successful
     */
    virtual bool CloseDevice() = 0;

    /**
     * Clears info from last frame.
     */
    virtual void Clear() = 0;

    class Input {
    public:
        virtual float GetState() = 0;
        virtual std::string GetName() = 0;
    };

protected:
    virtual std::string GetInputDeviceMapping() = 0;
};
