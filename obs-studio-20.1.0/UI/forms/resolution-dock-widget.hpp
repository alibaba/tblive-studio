#ifndef RESOLUTIONDOCKWIDGET_HPP
#define RESOLUTIONDOCKWIDGET_HPP

#include <QDockWidget>

namespace Ui {
class ResolutionDockWidget;
}

class ResolutionDockWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit ResolutionDockWidget(QWidget *parent = 0);
    ~ResolutionDockWidget();

private:
    Ui::ResolutionDockWidget *ui;
};

#endif // RESOLUTIONDOCKWIDGET_HPP
