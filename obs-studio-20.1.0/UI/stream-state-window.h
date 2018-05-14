#ifndef TBLIVESTREAMSTATETWINDOW_H
#define TBLIVESTREAMSTATETWINDOW_H

#include <QtWidgets/QMainWindow>
#include <QListWidget>

typedef struct tblive_system_info {
	QString cpu_name;
	QString system_ver;
	QString memory_info;
}tblive_system_info_t;

enum class StreamStateItem {
	CPU_NAME,
	CPU_USAGE,
	SYS_VER,
	MEM_INFO,
	FACE_BEAUTY_TIME,
	BAND_WIDTH,
	VIDEO_FRAME_RATE,
};

class StreamStateWindow : public QMainWindow
{
	Q_OBJECT

public:
	StreamStateWindow(tblive_system_info_t& info, QWidget *parent = 0);
	~StreamStateWindow();

public:
	void updateItems(StreamStateItem item, QString newValue);

private:
	void createItems();

private:
	tblive_system_info systemInfo;
	std::map<StreamStateItem, QListWidgetItem*> itemMaps;

	QListWidget *listWidget;

	QListWidgetItem* systemItem;
	QListWidgetItem* cpuNameItem;
	QListWidgetItem* cpuUsageItem;
	QListWidgetItem* memoryItem;
	QListWidgetItem* bandwidthItem;
	QListWidgetItem* faceBeautyTimeItem;
	QListWidgetItem* videoFPSItem;
	QListWidgetItem* tblsVersionItem;
};

#endif // TBLIVESTREAMSTATETWINDOW_H
