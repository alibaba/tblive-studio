#include "stream-url-dialog.h"
#include "ui_StreamUrlDialog.h"

StreamUrlDialog::StreamUrlDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::StreamUrlDialog)
{
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint);
}

StreamUrlDialog::~StreamUrlDialog()
{
    delete ui;
}

QString StreamUrlDialog::GetStreamUrl()
{
    return ui->lineEdit->text();
}
