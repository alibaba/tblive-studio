#pragma once

#include <QMainWindow>

#include "window-basic-main.hpp"

namespace Ui {
class OBSBasicStartPage;
}

class OBSBasicStartPage : public QMainWindow
{
    Q_OBJECT

public:
    explicit OBSBasicStartPage(QWidget *parent = 0);
    ~OBSBasicStartPage();

private slots:
    void on_new_live_btn_clicked();

private:
    Ui::OBSBasicStartPage *ui;
    OBSBasic *mainWind;
};
