#ifndef CHEATSEARCH_H
#define CHEATSEARCH_H

#include <QDialog>
#include <memory>
#include <functional>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLineEdit>

namespace Ui {
class CheatSearch;
}
class FoundItems;
class CheatSearch : public QDialog
{
    Q_OBJECT

public:
    explicit CheatSearch(QWidget *parent = 0);
    ~CheatSearch();

private:
    Ui::CheatSearch *ui;
    std::shared_ptr<std::vector<FoundItems>> previous_found;
    void OnScan(bool isNextScan);
    void OnScanTypeChanged(int index);
    void OnValueTypeChanged(int index);
    void OnHexCheckedChanged(bool checked);
    void LoadTable(std::shared_ptr<std::vector<FoundItems>> items);

    template <typename T, typename T2>
    std::shared_ptr<std::vector<FoundItems>> FirstSearch(const T value, std::function<bool(int, int, int)> comparer);
    std::shared_ptr<std::vector<FoundItems>> FirstSearch(const float value, std::function<bool(int, int, int)> comparer);
    std::shared_ptr<std::vector<FoundItems>> FirstSearch(const double value, std::function<bool(int, int, int)> comparer);

    template <typename T, typename T2>
    std::shared_ptr<std::vector<FoundItems>> NextSearch(const T value, std::function<bool(int, int, int)> comparer, const std::shared_ptr<std::vector<FoundItems>> previous_found);
    std::shared_ptr<std::vector<FoundItems>> NextSearch(const float value, std::function<bool(int, int, int)> comparer, const std::shared_ptr<std::vector<FoundItems>> previous_found);
    std::shared_ptr<std::vector<FoundItems>> NextSearch(const double value, std::function<bool(int, int, int)> comparer, const std::shared_ptr<std::vector<FoundItems>> previous_found);

    bool Equals(int a, int b, int c);

    bool LessThan(int a, int b, int c);

    bool GreaterThan(int a, int b, int c);

    bool Between(int min, int max, int value);
};
class ModifyAddressDialog : public QDialog {
    Q_OBJECT
public:
    ModifyAddressDialog(QWidget* parent, std::string address, int type, std::string value);
    ~ModifyAddressDialog();

    QString return_value;

private:
    QLineEdit* address_block;
    QComboBox* type_select;
    QLineEdit* value_block;

    void OnOkayClicked();
};
class FoundItems {
public:
    std::string address;
    std::string value;
};
#endif // CHEATSEARCH_H
