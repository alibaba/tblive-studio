#ifndef WINDOWBASICEND_H
#define WINDOWBASICEND_H

#include <QMainWindow>

#include "window-basic-main.hpp"

namespace Ui {
class OBSBasicEnd;
}

class OBSBasicEnd : public QMainWindow
{
    Q_OBJECT

public:
    explicit OBSBasicEnd(QWidget *parent = 0);
    ~OBSBasicEnd();

    void SetOBSBasic(OBSBasic * ob);
    void UpdateTitleBar();

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void on_live_data_btn_clicked();
    void on_new_live_btn_clicked();

private:
    Ui::OBSBasicEnd *ui;
    OBSBasic *mainWind;
};

#endif // WINDOWBASICEND_H
