#include "cheatsearch.h"
#include "ui_cheatsearch.h"

CheatSearch::CheatSearch(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CheatSearch)
{
    ui->setupUi(this);
}

CheatSearch::~CheatSearch()
{
    delete ui;
}
