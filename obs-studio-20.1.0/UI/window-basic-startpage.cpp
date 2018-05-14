#include <QCloseEvent>
#include <QDesktopServices>

#include "window-basic-startpage.hpp"
#include "ui_OBSBasicStartPage.h"

OBSBasicStartPage::OBSBasicStartPage(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::OBSBasicStartPage)
{
    ui->setupUi(this);
    setWindowTitle(NULL);
}

OBSBasicStartPage::~OBSBasicStartPage()
{
    delete ui;
}

void OBSBasicStartPage::on_new_live_btn_clicked()
{
    QDesktopServices::openUrl(QUrl("https://login.taobao.com/member/login.jhtml?\
        sub=true&redirectURL=http%3A%2F%2Fliveplatform.taobao.com%2Flive%2FliveDetail.htm"));

    close();
}
