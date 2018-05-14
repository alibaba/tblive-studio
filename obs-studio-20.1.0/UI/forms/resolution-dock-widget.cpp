#include "resolution-dock-widget.hpp"
#include "ui_ResolutionDockWidget.h"

ResolutionDockWidget::ResolutionDockWidget(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::ResolutionDockWidget)
{
    ui->setupUi(this);
}

ResolutionDockWidget::~ResolutionDockWidget()
{
    delete ui;
}
