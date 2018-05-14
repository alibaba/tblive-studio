#include <QMessageBox>

#include "bridge.h"

bridge::bridge()
{

}

bridge::~bridge()
{

}

// Slots for Web JavaScript Signals
void bridge::startStream(QString value)
{
	emit startStreamSucceedSignal(value);
}

void bridge::useElement(QString value)
{
    emit useElementSignal(value);
}

void bridge::uploadLogDone()
{
    emit uploadLogDoneSignal();
}

void bridge::login(QString value) {
	emit loginSucceedSignal(value);
}

void bridge::requestLoginAgain() {
    emit requestLoginAgainSignal();
}

void bridge::gotoLivePlatform() {
	emit requestOpenLivePlatform();
}

void bridge::uploadUserInfo(TBUserInfoType type, QString msg) {
    switch (type) {
        case TBUserInfoType::UserLogs:
            emit signalUploadUserLogsToJs(msg);
            break;
        case TBUserInfoType::CrashLogs:
            emit signalUploadCrashLogsToJs(msg);
            break;
        case TBUserInfoType::MonitorInfo:
            emit signalUploadMonitorInfoToJs(msg);
            break;
        default:
            break;
    }
}

void bridge::requestConfigs(QString uuid, QString type, QString param) {
    emit requestFromNative(uuid, type, param);
}

void bridge::acquireFromServer(QString uuid, QString value, QString status){
    emit tbliveBusinessResponseSignal(uuid, value, status);
}

void bridge::loadUrlDone(){
    emit loadUrlDoneSignal();
}

void bridge::openChildWin(QString win_url, int width, int height) {
    emit openChildWinSignal(win_url, width, height);
}

void bridge::closeChildWin(QString uuid, QString type, QString param) {
    emit closeChildWinSignal(uuid, type, param);
}

void bridge::deliverElementsParamsToJS(QString uuid, QString type, QString param) {
    emit requestFromNative(uuid, type, param);
}

