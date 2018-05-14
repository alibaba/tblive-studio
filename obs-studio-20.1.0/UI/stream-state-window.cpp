#include "stream-state-window.h"
#include <QStandardItemModel>
#include "obs-app.hpp"
#include "tblive-sdk-defines.h"

StreamStateWindow::StreamStateWindow(tblive_system_info_t& info, QWidget *parent)
	: QMainWindow(parent), systemInfo(info)
{
    setWindowTitle(QTStr("StreamStateWindow.Title"));
	setFixedSize(480, 300);

	listWidget = new QListWidget(this);
	createItems();
	setCentralWidget(listWidget);
}

StreamStateWindow::~StreamStateWindow()
{
	if (listWidget) {
		delete listWidget;
		listWidget = nullptr;
	}
}

void StreamStateWindow::createItems() {
	tblsVersionItem = new QListWidgetItem(QString::fromUtf8("TBLiveStudio Version: ").append(""));
	tblsVersionItem->setTextColor(Qt::white);
	listWidget->addItem(tblsVersionItem);

	systemItem = new QListWidgetItem(QString::fromUtf8("System: ").append(systemInfo.system_ver));
	systemItem->setTextColor(Qt::white);
	listWidget->addItem(systemItem);
	itemMaps.emplace(StreamStateItem::SYS_VER, systemItem);

	cpuNameItem = new QListWidgetItem(QString::fromUtf8("CPU: ").append(systemInfo.cpu_name));
	cpuNameItem->setTextColor(Qt::white);
	listWidget->addItem(cpuNameItem);
	itemMaps.emplace(StreamStateItem::CPU_NAME, cpuNameItem);

	cpuUsageItem = new QListWidgetItem(QString::fromUtf8("CPU Usage: "));
	cpuUsageItem->setTextColor(Qt::white);
	listWidget->addItem(cpuUsageItem);
	itemMaps.emplace(StreamStateItem::CPU_USAGE, cpuUsageItem);

	memoryItem = new QListWidgetItem(QString::fromUtf8("Memory: ").append(systemInfo.memory_info));
	memoryItem->setTextColor(Qt::white);
	listWidget->addItem(memoryItem);
	itemMaps.emplace(StreamStateItem::MEM_INFO, memoryItem);

	bandwidthItem = new QListWidgetItem(QString::fromUtf8("BandWidth: "));
	bandwidthItem->setTextColor(Qt::white);
	listWidget->addItem(bandwidthItem);
	itemMaps.emplace(StreamStateItem::BAND_WIDTH, bandwidthItem);

	faceBeautyTimeItem = new QListWidgetItem(QString::fromUtf8("FaceBeautyTime: "));
	faceBeautyTimeItem->setTextColor(Qt::white);
	listWidget->addItem(faceBeautyTimeItem);
	itemMaps.emplace(StreamStateItem::FACE_BEAUTY_TIME, faceBeautyTimeItem);

	videoFPSItem = new QListWidgetItem(QString::fromUtf8("VideoFPS: "));
	videoFPSItem->setTextColor(Qt::white);
	listWidget->addItem(videoFPSItem);
	itemMaps.emplace(StreamStateItem::VIDEO_FRAME_RATE, videoFPSItem);
}

void StreamStateWindow::updateItems(StreamStateItem item, QString newValue) {
	switch (item)
	{
	case StreamStateItem::CPU_USAGE:
		cpuUsageItem->setText(QString::fromUtf8("CPU Usage: ").append(newValue).append("%"));
		break;
	case StreamStateItem::MEM_INFO:
		memoryItem->setText(QString::fromUtf8("Memory: ").append(newValue));
		break;
	case StreamStateItem::FACE_BEAUTY_TIME:
		faceBeautyTimeItem->setText(QString::fromUtf8("FaceBeautyTime: ").append(newValue).append(" ms"));
		break;
	case StreamStateItem::BAND_WIDTH:
		bandwidthItem->setText(QString::fromUtf8("BandWidth: ").append(newValue).append(" kb/s"));
		break;
	case StreamStateItem::VIDEO_FRAME_RATE:
		videoFPSItem->setText(QString::fromUtf8("VideoFPS: ").append(newValue));
		break;
	default:
		break;
	}
}



