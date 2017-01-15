// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QKeyEvent>
#include <QWidget>

#include "core/settings.h"
#include "input_core/devices/device.h"
#include "ui_configure_input.h"

class QPushButton;
class QString;

namespace Ui {
class ConfigureInput;
}

class ConfigureInput : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureInput(QWidget* parent = nullptr);

    /// Save all button configurations to settings file
    void applyConfiguration();

private:
    std::unique_ptr<Ui::ConfigureInput> ui;
    std::map<Settings::NativeInput::Values, QPushButton*> button_map;
    std::map<QPushButton*, Settings::InputDeviceMapping> key_map;
    QPushButton* changing_button = nullptr; ///< button currently waiting for key press.
    QString previous_mapping;

    /// Load configuration settings into button text
    void loadConfiguration();

    /// Check all inputs for duplicate keys. Clears out any other button with the same value as this
    /// button's new value.
    void removeDuplicates(const Settings::InputDeviceMapping new_value);

    /// Handle keykoard key press event for input tab when a button is 'waiting'.
    void keyPressEvent(QKeyEvent* event) override;

    /// Convert key ASCII value to its' letter/name
    static QString getKeyName(Settings::InputDeviceMapping mapping);

    /// Set button text to name of key pressed.
    void setKey(Settings::InputDeviceMapping key_pressed);

    /// Event handler for all button released() event.
    void handleClick(QPushButton* sender);

    /// Restore all buttons to their default values.
    void restoreDefaults();

    /// Update UI to reflect current configuration.
    void updateButtonLabels();
};
