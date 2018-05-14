#include <QCloseEvent>
#include <fstream>
#include <sstream>

#include "window-basic-end.h"
#include "ui_OBSBasicEnd.h"

OBSBasicEnd::OBSBasicEnd(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::OBSBasicEnd)
{
    ui->setupUi(this);
    setWindowTitle(NULL);
}

OBSBasicEnd::~OBSBasicEnd()
{
    delete ui;
}

void OBSBasicEnd::closeEvent(QCloseEvent *event)
{
    QWidget::closeEvent(event);
    QMetaObject::invokeMethod(mainWind, "close", Qt::DirectConnection);
}

void OBSBasicEnd::SetOBSBasic(OBSBasic * ob)
{
    mainWind = ob;
}

void OBSBasicEnd::on_live_data_btn_clicked()
{
    close();
}

void OBSBasicEnd::on_new_live_btn_clicked()
{
    close();
}
