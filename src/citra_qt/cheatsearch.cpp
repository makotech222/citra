#include "cheatsearch.h"
#include "ui_cheatsearch.h"
#include "common/common_types.h"
#include "core/memory.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>
#include <QCheckBox>
#include <QTableWidgetItem>
#include <QVBoxLayout>

using namespace std;
CheatSearch::CheatSearch(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CheatSearch)
{
    setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint);
    setSizeGripEnabled(false);
    ui->setupUi(this);
    setFixedSize(size());
    ui->btnNextScan->setEnabled(false);
    ui->lblTo->setVisible(false);
    ui->txtSearchTo->setVisible(false);
    ui->tableFound->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableFound->setSelectionBehavior(QAbstractItemView::SelectRows);
    previous_found = make_shared<std::vector<FoundItems>>();
    connect(ui->btnNextScan, &QPushButton::released, this, [&]() {CheatSearch::OnScan(true); });
    connect(ui->btnFirstScan, &QPushButton::released, this, [&]() {CheatSearch::OnScan(false); });
    connect(ui->cbScanType, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [&](int index) {CheatSearch::OnScanTypeChanged(index); });
    connect(ui->cbValueType, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [&](int index) {CheatSearch::OnValueTypeChanged(index); });
    connect(ui->chkHex, &QCheckBox::clicked, this, [&](bool i) {CheatSearch::OnHexCheckedChanged(i); });
    connect(ui->tableFound, &QTableWidget::doubleClicked, this, [&](QModelIndex i) {
        ModifyAddressDialog* dialog = new ModifyAddressDialog(this, ui->tableFound->item(i.row(), 0)->text().toStdString(), ui->cbValueType->currentIndex(), ui->tableFound->item(i.row(), 1)->text().toStdString());
        dialog->exec();
        QString rv = dialog->return_value;
        ui->tableFound->item(i.row(), 1)->setText(rv);
    });
}

CheatSearch::~CheatSearch()
{
    delete ui;
}

string int_to_hex(int i)
{
    std::stringstream stream;
    stream << setfill('0') << setw(sizeof(int) * 2)
        << std::hex << i;
    return stream.str();
}

int hex_to_int(std::string hex_value)
{
    int decimal_value;
    std::stringstream ss;
    ss << hex_value; // std::string hex_value
    ss >> std::hex >> decimal_value; //int decimal_value
    return decimal_value;
}

double hexstr2double(const std::string& hexstr)
{
    union
    {
        long long i;
        double    d;
    } value;

    value.i = std::stoll(hexstr, nullptr, 16);

    return value.d;
}

std::string double2hexstr(double x) {

    union
    {
        long long i;
        double    d;
    } value;

    value.d = x;

    std::ostringstream buf;
    buf << std::hex << std::setfill('0') << std::setw(16) << value.i;

    return buf.str();

}

std::string ieee_float_to_hex(float f)
{
    union { float fval; std::uint32_t ival; };
    fval = f;

    std::ostringstream stm;
    stm << std::hex << std::uppercase << ival;

    return stm.str();
}

void CheatSearch::OnScan(bool isNextScan)
{
    int valueType;
    int searchType;
    QString searchvalue;
    bool convertHex;
    try
    {
        valueType = ui->cbValueType->currentIndex();
        searchType = ui->cbScanType->currentIndex();
        searchvalue = ui->txtSearch->text();
        convertHex = ui->chkHex->isChecked();
    }
    catch (const std::exception&)
    {
        ui->txtSearch->setText("");
        return;
    }
    std::function<bool(int, int, int)> comparer = [&](int a, int b, int c) {return CheatSearch::Equals(a, b, c); };

    switch (searchType) {
    case 0: //Equals
    {
        comparer = [&](int a, int b, int c) {return CheatSearch::Equals(a, b, c); };
        break;
    }
    case 1: //Greater Than
    {
        comparer = [&](int a, int b, int c) {return CheatSearch::GreaterThan(a, b, c); };
        break;
    }
    case 2: //Less Than
    {
        comparer = [&](int a, int b, int c) {return CheatSearch::LessThan(a, b, c); };
        break;
    }
    case 3: //Between
    {
        comparer = [&](int a, int b, int c) {return CheatSearch::Between(a, b, c); };
        break;
    }
    }
    int base = (ui->chkHex->isChecked()) ? 16 : 10;
    switch (valueType) {
    case 0: // int32
    {
        u32 value = searchvalue.toInt(nullptr, base);
        if (!isNextScan)
            previous_found = FirstSearch<u32, s32>(value, comparer);
        else
            previous_found = NextSearch<u32, s32>(value, comparer, previous_found);
        break;
    }
    case 1: // int16
    {
        u16 value = searchvalue.toInt(nullptr, base);
        if (!isNextScan)
            previous_found = FirstSearch<u16, s16>(value, comparer);
        else
            previous_found = NextSearch<u16, s16>(value, comparer, previous_found);
        break;
    }
    case 2: // int8
    {
        u8 value = searchvalue.toInt(nullptr, base);
        if (!isNextScan)
            previous_found = FirstSearch<u8, s8>(value, comparer);
        else
            previous_found = NextSearch<u8, s8>(value, comparer, previous_found);
        break;
    }
    case 3: // float
    {
        float value = searchvalue.toFloat();
        if (!isNextScan)
            previous_found = FirstSearch(value, comparer);
        else
            previous_found = NextSearch(value, comparer, previous_found);
        break;
    }
    case 4: // double
    {
        double value = searchvalue.toDouble();
        if (!isNextScan)
            previous_found = FirstSearch(value, comparer);
        else
            previous_found = NextSearch(value, comparer, previous_found);
        break;
    }
    }
    ui->tableFound->setRowCount(0);
    if (previous_found->size() > 50000) {
        ui->lblCount->setText(QString::fromStdString("Found: 50000+"));
    }
    else {
        LoadTable(previous_found);
        ui->lblCount->setText(QString::fromStdString("Found: " + to_string(previous_found->size())));
    }

    ui->btnNextScan->setEnabled(previous_found->size() > 0);
}

void CheatSearch::OnValueTypeChanged(int index)
{
    ui->txtSearch->setText("");
    ui->txtSearchTo->setText("");
    if (index >= 0 && index <= 2)
    {
        ui->chkHex->setVisible(true);
    }
    else {
        ui->chkHex->setVisible(false);
        ui->chkHex->setChecked(false);
    }
}

void CheatSearch::OnScanTypeChanged(int index)
{
    if (index == 3) // Between
    {
        ui->lblTo->setVisible(true);
        ui->txtSearchTo->setVisible(true);
    }
    else {
        ui->lblTo->setVisible(false);
        ui->txtSearchTo->setVisible(false);
        ui->txtSearchTo->setText("");
    }
    if (index == 0) { // Equals
        ui->chkNot->setVisible(true);
    }
    else {
        ui->chkNot->setVisible(false);
        ui->chkNot->setChecked(false);
    }
}

void CheatSearch::OnHexCheckedChanged(bool checked)
{
    std::string text = ui->txtSearch->text().toStdString();
    std::string text2 = ui->txtSearchTo->text().toStdString();
    try
    {
        if (checked) {
            if (text.length() > 0) {
                int val = std::stoi(text, nullptr, 10);
                ui->txtSearch->setText(QString::fromStdString(int_to_hex(val)));
            }
            if (text2.length() > 0) {
                int val2 = std::stoi(text2, nullptr, 10);
                ui->txtSearchTo->setText(QString::fromStdString(int_to_hex(val2)));
            }
        }
        else {
            if (text.length() > 0) {
                ui->txtSearch->setText(QString::fromStdString(to_string(hex_to_int(text))));
            }
            if (text2.length() > 0) {
                ui->txtSearchTo->setText(QString::fromStdString(to_string(hex_to_int(text2))));
            }
        }
    }
    catch (const std::exception&)
    {
        ui->txtSearch->setText("");
        ui->txtSearchTo->setText("");
    }
}

void CheatSearch::LoadTable(std::shared_ptr<std::vector<FoundItems>> items)
{
    ui->tableFound->setRowCount(items->size());
    for (int i = 0; i < items->size(); i++) {
        ui->tableFound->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(items->at(i).address)));
        ui->tableFound->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(items->at(i).value)));
        ui->tableFound->setRowHeight(i, 23);
    }
}

template <typename T, typename T2>
shared_ptr<vector<FoundItems>> CheatSearch::FirstSearch(const T value, std::function<bool(int, int, int)> comparer) {
    u32 start_address = 0x00000000;
    u32 end_address = 0x08000000 + 0x08000000;
    vector<int> address_in_use;
    shared_ptr<vector<FoundItems>> results = make_shared<vector<FoundItems>>();
    int base = (ui->chkHex->isChecked()) ? 16 : 10;
    int searchToValue = ui->txtSearchTo->text().toInt(nullptr, base);

    for (u32 i = start_address; i < end_address; i += 4096) {
        if (Memory::IsValidVirtualAddress(i)) {
            address_in_use.push_back(i);
        }
    }
    for (auto& range : address_in_use) {
        for (u32 i = range; i < range + 4096; i++) {
            T resultTemp = Memory::Read<T>(i);
            T2 result = (T2)resultTemp;
            if (comparer(result, value, searchToValue)) {
                FoundItems item = FoundItems();
                item.address = int_to_hex(i);
                item.value = to_string(result);
                results->push_back(item);
            }
        }
    }
    return results;
}

std::shared_ptr<std::vector<FoundItems>> CheatSearch::FirstSearch(const float value, std::function<bool(int, int, int)> comparer)
{
    u32 start_address = 0x00000000;
    u32 end_address = 0x08000000 + 0x08000000;
    vector<int> address_in_use;
    shared_ptr<vector<FoundItems>> results = make_shared<vector<FoundItems>>();
    int base = (ui->chkHex->isChecked()) ? 16 : 10;
    int searchToValue = ui->txtSearchTo->text().toInt(nullptr, base);

    for (u32 i = start_address; i < end_address; i += 4096) {
        if (Memory::IsValidVirtualAddress(i)) {
            address_in_use.push_back(i);
        }
    }
    for (auto& range : address_in_use) {
        for (u32 i = range; i < range + 4096; i++) {
            float result = Memory::ReadFloat(i);
            if (comparer(result, value, searchToValue)) {
                FoundItems item = FoundItems();
                item.address = int_to_hex(i);
                item.value = to_string(result);
                results->push_back(item);
            }
        }
    }
    return results;
}

std::shared_ptr<std::vector<FoundItems>> CheatSearch::FirstSearch(const double value, std::function<bool(int, int, int)> comparer)
{
    u32 start_address = 0x00000000;
    u32 end_address = 0x08000000 + 0x08000000;
    vector<int> address_in_use;
    shared_ptr<vector<FoundItems>> results = make_shared<vector<FoundItems>>();
    int base = (ui->chkHex->isChecked()) ? 16 : 10;
    int searchToValue = ui->txtSearchTo->text().toInt(nullptr, base);

    for (u32 i = start_address; i < end_address; i += 4096) {
        if (Memory::IsValidVirtualAddress(i)) {
            address_in_use.push_back(i);
        }
    }
    for (auto& range : address_in_use) {
        for (u32 i = range; i < range + 4096; i++) {
            double result = Memory::ReadDouble(i);
            if (comparer(result, value, searchToValue)) {
                FoundItems item = FoundItems();
                item.address = int_to_hex(i);
                item.value = to_string(result);
                results->push_back(item);
            }
        }
    }
    return results;
}

template<typename T, typename T2>
shared_ptr<vector<FoundItems>> CheatSearch::NextSearch(const T value, std::function<bool(int, int, int)> comparer, shared_ptr<vector<FoundItems>> previousFound)
{
    shared_ptr<vector<FoundItems>> results = make_shared<vector<FoundItems>>();
    int base = (ui->chkHex->isChecked()) ? 16 : 10;
    int searchToValue = ui->txtSearchTo->text().toInt(nullptr, base);
    for (auto& f : *previousFound) {
        int addr = std::stoul(f.address, nullptr, 16);
        T resultTemp = Memory::Read<T>(addr);
        T2 result = (T2)resultTemp;
        if (comparer(result, value, searchToValue)) {
            FoundItems item = FoundItems();
            item.address = int_to_hex(addr);
            item.value = to_string(result);
            results->push_back(item);
        }
    }

    return results;
}

shared_ptr<vector<FoundItems>> CheatSearch::NextSearch(const float value, std::function<bool(int, int, int)> comparer, const shared_ptr<vector<FoundItems>> previousFound)
{
    shared_ptr<vector<FoundItems>> results = make_shared<vector<FoundItems>>();
    int base = (ui->chkHex->isChecked()) ? 16 : 10;
    int searchToValue = ui->txtSearchTo->text().toInt(nullptr, base);
    for (auto& f : *previousFound) {
        int addr = std::stoul(f.address, nullptr, 16);
        float result = Memory::ReadFloat(addr);
        if (comparer(result, value, searchToValue)) {
            FoundItems item = FoundItems();
            item.address = int_to_hex(addr);
            item.value = to_string(result);
            results->push_back(item);
        }
    }

    return results;
}

shared_ptr<vector<FoundItems>> CheatSearch::NextSearch(const double value, std::function<bool(int, int, int)> comparer, const shared_ptr<vector<FoundItems>> previousFound)
{
    shared_ptr<vector<FoundItems>> results = make_shared<vector<FoundItems>>();
    int base = (ui->chkHex->isChecked()) ? 16 : 10;
    int searchToValue = ui->txtSearchTo->text().toInt(nullptr, base);
    for (auto& f : *previousFound) {
        int addr = std::stoul(f.address, nullptr, 16);
        double result = Memory::ReadDouble(addr);
        if (comparer(result, value, searchToValue)) {
            FoundItems item = FoundItems();
            item.address = int_to_hex(addr);
            item.value = to_string(result);
            results->push_back(item);
        }
    }

    return results;
}

bool CheatSearch::Equals(int a, int b, int c)
{
    if (ui->chkNot->isChecked())
        return a != b;
    else
        return a == b;
}

bool CheatSearch::LessThan(int a, int b, int c)
{
    return a < b;
}

bool CheatSearch::GreaterThan(int a, int b, int c)
{
    return a > b;
}

bool CheatSearch::Between(int value, int min, int max)
{
    return min < value && value < max;
}

ModifyAddressDialog::ModifyAddressDialog(QWidget* parent, string address, int type, string value) : QDialog(parent) {
    resize(450, 150);
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
    setSizeGripEnabled(false);
    auto mainLayout = new QVBoxLayout(this);

    QHBoxLayout* editPanel = new QHBoxLayout();
    address_block = new QLineEdit();
    value_block = new QLineEdit();
    type_select = new QComboBox();

    address_block->setReadOnly(true);
    address_block->setText(QString::fromStdString(address));
    address_block->setFixedWidth(100);

    type_select->addItem("Int32");
    type_select->addItem("Int16");
    type_select->addItem("Int8");
    type_select->addItem("Float");
    type_select->addItem("Double");
    type_select->setCurrentIndex(type);
    type_select->setFixedWidth(100);

    value_block->setText(QString::fromStdString(value));
    value_block->setFixedWidth(100);

    editPanel->addWidget(address_block);
    editPanel->addWidget(type_select);
    editPanel->addWidget(value_block);

    auto button_box = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(button_box, &QDialogButtonBox::accepted, this, [=]() { OnOkayClicked(); });
    QHBoxLayout* confirmationPanel = new QHBoxLayout();
    confirmationPanel->addWidget(button_box);
    mainLayout->addLayout(editPanel);
    mainLayout->addLayout(confirmationPanel);
}

ModifyAddressDialog::~ModifyAddressDialog()
{
}

void ModifyAddressDialog::OnOkayClicked()
{
    int valueType;
    QString newValue;
    int address;
    try
    {
        valueType = type_select->currentIndex();
        newValue = value_block->text();
        address = address_block->text().toInt(nullptr, 16);
    }
    catch (const std::exception&)
    {
        this->close();
    }

    int base = 10;
    switch (valueType) {
    case 0: // int32
    {
        u32 value = newValue.toInt(nullptr, base);
        Memory::Write32(address, value);
        break;
    }
    case 1: // int16
    {
        u16 value = newValue.toInt(nullptr, base);
        Memory::Write16(address, value);
        break;
    }
    case 2: // int8
    {
        u8 value = newValue.toInt(nullptr, base);
        Memory::Write8(address, value);
        break;
    }
    case 3: // float
    {
        float value = newValue.toFloat();
        u32 converted = stoi(ieee_float_to_hex(value), nullptr, 10);
        Memory::Write32(address, converted);
        break;
    }
    case 4: // double
    {
        double value = newValue.toDouble();
        u64 converted = strtoull(double2hexstr(value).c_str(), nullptr, 10);
        Memory::Write64(address, converted);
        break;
    }
    }
    return_value = newValue;
    this->close();
}
