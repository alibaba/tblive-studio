#include "window-hoverwidget.hpp"
#include "window-basic-main.hpp"
#include "obs-app.hpp"
#include "qt-wrappers.hpp"

//#include "tblive_sdk/tblive_sdk.h"
//#include "base/basictypes.h"

#if defined(_WIN32)
#include <windows.h>
#endif

namespace {

bool IsChildWidget(QWidget * child, QWidget * parent)
{
	QWidget * childParent = child;
	while ( childParent )
	{
		if ( childParent == parent )
		{
			return true;
		}
		else
		{
			childParent = childParent->parentWidget();
		}
	}

	return false;
}

}// namespace

VolHoverWidget::VolHoverWidget(QWidget *parent)
	: QDockWidget(parent, Qt::FramelessWindowHint),
	  ui(new Ui::HoverWidget),
	  main(nullptr)
{
	ui->setupUi(this);

	QWidget * emptyTitle = new QWidget(this);
	setTitleBarWidget(emptyTitle);

	// focus
	setFocusPolicy(Qt::StrongFocus);

#if defined(_WIN32)
	SetWindowPos((HWND)winId(), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
#endif
	
	ImplInstallEventFilter();
}

void VolHoverWidget::ImplInstallEventFilter()
{
#if defined(_WIN32)
	ui->scrollArea->installEventFilter(this);
	ui->volumeWidgets->installEventFilter(this);
#else
    QWidget::installEventFilter(this);
#endif
}

bool VolHoverWidget::eventFilter(QObject *target, QEvent *event)
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

void VolHoverWidget::RecheckInputOutputAudioSrc()
{
	//枚举设备
	//tblive_sdk::check_add_all_audio_sources_to_current_scene();
}

void VolHoverWidget::ShowAt(int px, int py)
{
	show();
	//水平方向以px为中点显示
	move(px - width() / 2, py - height() -5);

	activateWindow();
}

void VolHoverWidget::AddWidget2Layout(QWidget *w)
{
	ui->volumeWidgets->layout()->addWidget(w);
    
#if defined(_WIN32)
	w->installEventFilter(this);
	QList<QWidget *> pWidgetList = w->findChildren<QWidget *>();
	for (int i = 0; i < pWidgetList.count(); i++)
	{ 
		pWidgetList.at(i)->installEventFilter(this);
	}
#endif
}

void VolHoverWidget::SetOBSBasic(OBSBasic * ob)
{
	main = ob;
}

void VolHoverWidget::on_audioMixerBtn_clicked()
{
	if ( main )
	{
		main->OnAudioMixerBtn();
	}
}

void VolHoverWidget::on_scrollArea_customContextMenuRequested()
{
    QMetaObject::invokeMethod(main, "on_mixerScrollArea_customContextMenuRequested",
                              Qt::DirectConnection);
}
