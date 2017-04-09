#ifndef CHEATSEARCH_H
#define CHEATSEARCH_H

#include <QDialog>

namespace Ui {
class CheatSearch;
}

class CheatSearch : public QDialog
{
    Q_OBJECT

public:
    explicit CheatSearch(QWidget *parent = 0);
    ~CheatSearch();

private:
    Ui::CheatSearch *ui;
};

#endif // CHEATSEARCH_H
