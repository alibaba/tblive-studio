#ifndef RESOLUTIONDOCKWIDGET_HPP
#define RESOLUTIONDOCKWIDGET_HPP

#include <QDockWidget>

class OBSBasic;

namespace Ui {
class ResolutionDockWidget;
}

class ResolutionDockWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit ResolutionDockWidget(QWidget *parent = 0);
    ~ResolutionDockWidget();

    void ShowAt(int px, int py);
    void SetOBSBasic(OBSBasic * ob);

protected:
    bool eventFilter(QObject *target, QEvent *event);

private slots:
    void onRadioClick();

private:
    Ui::ResolutionDockWidget *ui;
    OBSBasic *main;
};

#endif // RESOLUTIONDOCKWIDGET_HPP
