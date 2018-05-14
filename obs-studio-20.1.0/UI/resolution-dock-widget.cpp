#include "resolution-dock-widget.hpp"
#include "ui_ResolutionDockWidget.h"
#include "window-basic-main.hpp"

ResolutionDockWidget::ResolutionDockWidget(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::ResolutionDockWidget)
{
    ui->setupUi(this);

    ui->radioButton_720p->setStyleSheet(
        "QRadioButton::indicator{ width: 110px; height: 54px;}"
        "QRadioButton::indicator::unchecked {image: url(:/res/icons/720p-unselected.png);}"
        "QRadioButton::indicator::checked {image: url(:/res/icons/720p-selected.png);}"
    );

    ui->radioButton_360p->setStyleSheet(
        "QRadioButton::indicator{ width: 110px; height: 54px;}"
        "QRadioButton::indicator::unchecked {image: url(:/res/icons/360p-unselected.png);}"
        "QRadioButton::indicator::checked {image: url(:/res/icons/360p-selected.png);}"
    );

    setWindowFlags(Qt::FramelessWindowHint);
    QWidget::installEventFilter(this);

    ui->radioButton_720p->setChecked(true);

    connect(ui->radioButton_720p, SIGNAL(clicked()), this, SLOT(onRadioClick()));
    connect(ui->radioButton_360p, SIGNAL(clicked()), this, SLOT(onRadioClick()));
}


ResolutionDockWidget::~ResolutionDockWidget()
{                                                                                                                                                                                                                                     
    delete ui;
}

void ResolutionDockWidget::SetOBSBasic(OBSBasic * ob)
{
    main = ob;
}

void ResolutionDockWidget::onRadioClick()
{
    if (ui->radioButton_720p->isChecked()) {
        main->m_is720p = true;
        main->SetStreamParams();
        main->UpdataResolutionIcon();
    }

    if (ui->radioButton_360p->isChecked()) {
        main->m_is720p = false;
        main->SetStreamParams();
        main->UpdataResolutionIcon();
    }

    main->ResetVideo();
    main->ResetAudio();
}

void ResolutionDockWidget::ShowAt(int px, int py)
{
    show();
    //水平方向以px为中点显示
    move(px - width() / 2, py - height() -5);

    activateWindow();
}

bool ResolutionDockWidget::eventFilter(QObject *target, QEvent *event)
{
    //窗口停用，变为不活动的窗口
    if (QEvent::WindowDeactivate == event->type())
    {
        QWidget * activeWnd = QApplication::activeWindow();
        if ( activeWnd == this ) {
            return false;
        }

        hide();
        return true;
    }

    return false;
}
