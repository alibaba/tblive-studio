#ifndef BRIDGE_H
#define BRIDGE_H

#include <QObject>
#include <QJsonValue>
#include <QUuid>

enum class TBUserInfoType {
    UserLogs,
    CrashLogs,
    MonitorInfo,
};

class bridge : public QObject
{
	Q_OBJECT

public:
	bridge();
	~bridge();

public:
    void uploadUserInfo(TBUserInfoType type, QString msg);
    void requestConfigs(QString uuid, QString type, QString param);
    void deliverElementsParamsToJS(QString uuid, QString type, QString param);

    //对内向native发送的信号
signals:
    void startStreamSucceedSignal(QJsonValue value);
    void loginSucceedSignal(QJsonValue value);
    void requestLoginAgainSignal();
    void useElementSignal(QJsonValue value);
    void uploadLogDoneSignal();
    void requestOpenLivePlatform();
    void loadUrlDoneSignal();
	void tbliveBusinessResponseSignal(QString uuid, QString value, QString status);
    void openChildWinSignal(QString win_url, int width, int height);
    void closeChildWinSignal(QString uuid, QString type, QString param);
    
    //对外向JS发送的信号
signals:
    void signalLoginSucceedToJs(QString msg);
    void signalUploadMonitorInfoToJs(QString msg);
    void signalUploadUserLogsToJs(QString msg);
    void signalUploadCrashLogsToJs(QString msg);
    void signalRequestConfigs(QString uuid);
    void requestFromNative(QString uuid, QString type, QString param);

    //会被
public slots:
    void startStream(QString value);
    void login(QString value);
    void requestLoginAgain();
    void useElement(QString value);
    void uploadLogDone();
    void gotoLivePlatform();
    void loadUrlDone();
    void acquireFromServer(QString uuid, QString value, QString status);
    
    // 素材页面各种窗口调用触发
public slots:
    void openChildWin(QString win_url, int width = 300, int height = 300);
    void closeChildWin(QString uuid, QString type, QString param);
    

private:
    QUuid mUuid;
};

#endif // BRIDGE_H
