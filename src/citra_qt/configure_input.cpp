// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <utility>
#include "citra_qt/config.h"
#include "citra_qt/configure_input.h"
#include "citra_qt/keybinding_names.h"
#include "common/string_util.h"
#include "input_core/devices/keyboard.h"
#include "input_core/input_core.h"

ConfigureInput::ConfigureInput(QWidget* parent)
    : QWidget(parent), ui(std::make_unique<Ui::ConfigureInput>()) {

    ui->setupUi(this);

    // Initialize mapping of input enum to UI button.
    button_map = {
        {Settings::NativeInput::Values::A, ui->buttonA},
        {Settings::NativeInput::Values::B, ui->buttonB},
        {Settings::NativeInput::Values::X, ui->buttonX},
        {Settings::NativeInput::Values::Y, ui->buttonY},
        {Settings::NativeInput::Values::L, ui->buttonL},
        {Settings::NativeInput::Values::R, ui->buttonR},
        {Settings::NativeInput::Values::ZL, ui->buttonZL},
        {Settings::NativeInput::Values::ZR, ui->buttonZR},
        {Settings::NativeInput::Values::START, ui->buttonStart},
        {Settings::NativeInput::Values::SELECT, ui->buttonSelect},
        {Settings::NativeInput::Values::HOME, ui->buttonHome},
        {Settings::NativeInput::Values::DUP, ui->buttonDpadUp},
        {Settings::NativeInput::Values::DDOWN, ui->buttonDpadDown},
        {Settings::NativeInput::Values::DLEFT, ui->buttonDpadLeft},
        {Settings::NativeInput::Values::DRIGHT, ui->buttonDpadRight},
        {Settings::NativeInput::Values::CUP, ui->buttonCStickUp},
        {Settings::NativeInput::Values::CDOWN, ui->buttonCStickDown},
        {Settings::NativeInput::Values::CLEFT, ui->buttonCStickLeft},
        {Settings::NativeInput::Values::CRIGHT, ui->buttonCStickRight},
        {Settings::NativeInput::Values::CIRCLE_UP, ui->buttonCircleUp},
        {Settings::NativeInput::Values::CIRCLE_DOWN, ui->buttonCircleDown},
        {Settings::NativeInput::Values::CIRCLE_LEFT, ui->buttonCircleLeft},
        {Settings::NativeInput::Values::CIRCLE_RIGHT, ui->buttonCircleRight},
    };

    // Attach handle click method to each button click.
    for (const auto& entry : button_map) {
        connect(entry.second, &QPushButton::released, [=]() { handleClick(entry.second); });
    }
    connect(ui->buttonCircleMod, &QPushButton::released,
            [=]() { handleClick(ui->buttonCircleMod); });
    connect(ui->buttonRestoreDefaults, &QPushButton::released, [=]() { restoreDefaults(); });

    setFocusPolicy(Qt::ClickFocus);

    this->loadConfiguration();
}

void ConfigureInput::handleClick(QPushButton* sender) {
    if (sender == nullptr)
        return;
    previous_mapping = sender->text();
    sender->setText(tr("[press key]"));
    sender->setFocus();
    grabKeyboard();
    grabMouse();
    changing_button = sender;
    auto update = []() { QCoreApplication::processEvents(); };
    auto input_device = InputCore::DetectInput(5000, update);

    setKey(input_device);
}

void ConfigureInput::keyPressEvent(QKeyEvent* event) {
    if (!changing_button)
        return;
    if (!event || event->key() == Qt::Key_unknown)
        return;

    auto keyboard = InputCore::GetKeyboard();
    KeyboardKey param =
        KeyboardKey(event->key(), QKeySequence(event->key()).toString().toStdString());
    keyboard->KeyPressed(param);
}

void ConfigureInput::applyConfiguration() {
    for (int i = 0; i < Settings::NativeInput::NUM_INPUTS; ++i) {
        Settings::values.input_mappings[Settings::NativeInput::All[i]] =
            key_map[button_map[Settings::NativeInput::Values(i)]];
    }
    Settings::values.pad_circle_modifier = key_map[ui->buttonCircleMod];
    Settings::Apply();
}

void ConfigureInput::loadConfiguration() {
    for (int i = 0; i < Settings::NativeInput::NUM_INPUTS; ++i) {
        key_map[button_map[Settings::NativeInput::Values(i)]] = Settings::values.input_mappings[i];
    }
    key_map[ui->buttonCircleMod] = Settings::values.pad_circle_modifier;
    updateButtonLabels();
}

void ConfigureInput::setKey(Settings::InputDeviceMapping key_pressed) {
    if (key_pressed.key == -1 || key_pressed.key == Qt::Key_Escape) {
    } else {
        key_map[changing_button] = key_pressed;
        removeDuplicates(key_pressed);
    }
    updateButtonLabels();
    releaseKeyboard();
    releaseMouse();
    changing_button = nullptr;
    previous_mapping = nullptr;
}

QString ConfigureInput::getKeyName(Settings::InputDeviceMapping mapping) {
    if (mapping.key == -1)
        return "";
    if (mapping.device == Settings::Device::Gamepad) {
        if (KeyBindingNames::sdl_gamepad_names.size() > mapping.key && mapping.key >= 0)
            return KeyBindingNames::sdl_gamepad_names[mapping.key];
        else
            return "";
    }
    if (mapping.key == Qt::Key_Shift)
        return tr("Shift");
    if (mapping.key == Qt::Key_Control)
        return tr("Ctrl");
    if (mapping.key == Qt::Key_Alt)
        return tr("Alt");
    if (mapping.key == Qt::Key_Meta)
        return "";
    if (mapping.key < 0)
        return "";

    return QKeySequence(mapping.key).toString();
}

void ConfigureInput::removeDuplicates(const Settings::InputDeviceMapping new_value) {
    for (auto& entry : key_map) {
        if (changing_button != entry.first) {
            if (new_value == entry.second && new_value.key == entry.second.key) {
                entry.second = Settings::InputDeviceMapping();
            }
        }
    }
}

void ConfigureInput::restoreDefaults() {
    for (int i = 0; i < Settings::NativeInput::NUM_INPUTS; ++i) {
        Settings::InputDeviceMapping mapping =
            Settings::InputDeviceMapping(Config::defaults[i].toInt());
        key_map[button_map[Settings::NativeInput::Values(i)]] = mapping;
        const QString keyValue =
            getKeyName(Settings::InputDeviceMapping(Config::defaults[i].toInt()));
    }
    key_map[ui->buttonCircleMod] =
        Settings::InputDeviceMapping(Config::default_circle_pad_modifier.toInt());
    updateButtonLabels();
}

void ConfigureInput::updateButtonLabels() {
    for (const auto& mapping : button_map) {
        auto button = mapping.second;
        button->setText(getKeyName(key_map[button]));
    }
    ui->buttonCircleMod->setText(getKeyName(key_map[ui->buttonCircleMod]));
}
