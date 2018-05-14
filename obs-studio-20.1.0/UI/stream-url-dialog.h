#ifndef STREAMURLDIALOG_H
#define STREAMURLDIALOG_H

#include <QDialog>

namespace Ui {
class StreamUrlDialog;
}

class StreamUrlDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StreamUrlDialog(QWidget *parent = 0);
    ~StreamUrlDialog();

    QString GetStreamUrl();

private:
    Ui::StreamUrlDialog *ui;
};

#endif // STREAMURLDIALOG_H
