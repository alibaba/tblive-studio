
#include <time.h>
#include <obs.hpp>
#include <QMessageBox>
#include <QShowEvent>
#include <QDesktopServices>
#include <QFileDialog>
#include <QDesktopWidget>
#include <QUrl>
#include <QUrlQuery>
#include <util/dstr.h>
#include <util/util.hpp>
#include <util/platform.h>
#include <util/profiler.hpp>

#include "commonToolFun.hpp"
#include "obs-app.hpp"
#include "platform.hpp"
#include "resolution-dock-widget.hpp"
#include "stream-url-dialog.h"
#include "visibility-item-widget.hpp"
#include "item-widget-helpers.hpp"
#include "window-basic-settings.hpp"
#include "window-basic-source-select.hpp"
#include "window-basic-main.hpp"
#include "window-basic-main-outputs.hpp"
#include "window-basic-properties.hpp"
#include "window-hoverwidget.hpp"
#include "window-log-reply.hpp"
#include "window-namedialog.hpp"
#include "window-projector.hpp"
#include "window-remux.hpp"
#include "qt-wrappers.hpp"
#include "display-helpers.hpp"
#include "volume-control.hpp"
#include "remote-text.hpp"
#include <stdlib.h>

#include "tblive-sdk.h"

#include "ui_OBSBasic.h"

#if defined(_WIN32)
#include <windowsx.h>
#endif

//#include <QWebEngineProfile>
//#include <QWebEngineCookieStore>
//
//static bool g_need = false;

BPtr<char> ReadLogFile(const char *log);

// OBSBasic

void OBSBasic::SetupStreamStateWindow() {
	tblive_system_info_t tblive_info;
	obs_system_info_t obs_info ;
	memset(&obs_info, 0, sizeof(obs_system_info_t));
	obs_query_system_info(&obs_info);
#if defined(_WIN32)
	tblive_info.cpu_name = QString::fromWCharArray(obs_info.cpu_name, 1024);
#else
	tblive_info.cpu_name = QString(obs_info.cpu_name);
#endif

	tblive_info.system_ver = QString(obs_info.system_name);
	tblive_info.memory_info = QString(obs_info.memory_info);

	stateWindow = new StreamStateWindow(tblive_info, this);

	stateWindowUpdateTimer = new QTimer(this);
	connect(stateWindowUpdateTimer, SIGNAL(timeout()), this, SLOT(updateStateWindow()));
	stateWindowUpdateTimer->start(3000);
}

void OBSBasic::updateStateWindow() {
	if (!stateWindow.isNull()) {
		stateWindow->updateItems(StreamStateItem::BAND_WIDTH, QString::number(outputBandwidth));
		stateWindow->updateItems(StreamStateItem::CPU_USAGE, QString::number(GetCPUUsage(), 'f', 1));
		stateWindow->updateItems(StreamStateItem::VIDEO_FRAME_RATE, QString::number(obs_get_active_fps()));
		stateWindow->updateItems(StreamStateItem::FACE_BEAUTY_TIME, QString::number(obs_get_face_beauty_time()));

		obs_system_info_t info;
		memset(&info, 0, sizeof(obs_system_info_t));
		obs_query_system_info(&info);
		stateWindow->updateItems(StreamStateItem::MEM_INFO, QString(info.memory_info));
	}
}

void OBSBasic::InitTbliveUI()
{
    InitHoverVolWidget();
    InitResolutionSelectWidget();

	SetupStreamStateWindow();

    ui->startStreamBtn->setVisible(true);
    ui->stopStreamBtn->setVisible(false);
    ui->elementsBtn->setVisible(false);

    ui->previewDisabledLabel->setVisible(false);
    ui->label_titlebar_nick->setVisible(false);
    ui->label_titlebar_portrait->setVisible(false);

	QString commentViewUrl = TBLiveEngine::Instance()->GetUrl(TBLSDK_COMMENT_VIEW_URL);
    commentViewUrl.append(QString::fromStdString(App()->liveTopic));
	ui->commentView->load(QUrl(commentViewUrl));

    elementView = new TBLiveElementsView(this);
	QObject::connect(elementView, &TBLiveElementsView::requestLoginAgain, [this]{
	            blog(LOG_INFO, "Login again because of the timeout of login when config elements animations.");
	            App()->createAndShowLoginWindow(false);
	});
	QObject::connect(elementView, &TBLiveElementsView::createSourceSignal, this, &OBSBasic::AddElementSource);
    ui->elementView->page()->setWebChannel(elementView->m_webChannel);
	//if (!g_need) {
	//	ui->elementView->page()->profile()->cookieStore()->deleteAllCookies();
	//	g_need = true;
	//}
	ui->elementView->load(QUrl(TBLiveEngine::Instance()->GetUrl(TBLSDK_ELEMENT_VIEW_URL)));
    ui->beautifyBtn->setToolTip(QTStr("FaceBeautify.Opened"));
    ui->beautifyBtn->setStyleSheet("QPushButton { \
                                   background-image:url(:/res/icons/face-hover.png); \
                                   background-position: center; \
                                   background-repeat: no-repeat; \
                                   background-color: transparent; \
                                   }");

    //hide menu
    ui->menu_File->menuAction()->setVisible(false);
    ui->menuBasic_MainMenu_Edit->menuAction()->setVisible(false);
    ui->viewMenu->menuAction()->setVisible(false);
    ui->profileMenu->menuAction()->setVisible(false);
    ui->sceneCollectionMenu->menuAction()->setVisible(false);
    //ui->menuBasic_MainMenu_Help->menuAction()->setVisible(false);
    ui->menuTools->menuAction()->setVisible(false);
}

void OBSBasic::InitHoverVolWidget()
{
	if (m_pHoverVolCtrlWidget == nullptr)
	{
        m_pHoverVolCtrlWidget.reset();
        m_pHoverVolCtrlWidget.reset(new VolHoverWidget(NULL));
		m_pHoverVolCtrlWidget->SetOBSBasic(this);
		m_pHoverVolCtrlWidget->hide();
	}
}

void OBSBasic::InitResolutionSelectWidget()
{
    if (m_pReslSelWidget == nullptr)
    {
        m_pReslSelWidget.reset();
        m_pReslSelWidget.reset(new ResolutionDockWidget(NULL));
        m_pReslSelWidget->SetOBSBasic(this);
        m_pReslSelWidget->hide();
    }
}

const char* OBSBasic::GetSourceNameWithID(const char *id)
{
    const char *src_name = obs_source_get_display_name(id);
    OBSScene scene = GetCurrentScene();

    if (!scene || !src_name)
        return nullptr;

    obs_source_t * source = nullptr;
    QString placeHolderText{QT_UTF8(src_name)};

    QString text{placeHolderText};
    int i = 2;
    while ((source = obs_get_source_by_name(QT_TO_UTF8(text)))) {
        obs_source_release(source);
        text = QString("%1 %2").arg(placeHolderText).arg(i++);
    }

    return QT_TO_UTF8(text);
}

void OBSBasic::OnAudioMixerBtn()
{
    on_advAudioProps_clicked();
}

void OBSBasic::on_volCtrlBtn_released()
{
    QPoint pt = QCursor::pos();

    //悬浮在鼠标上方的位置
    pt.setY(pt.ry() - ui->volCtrlBtn->height() / 2);
    m_pHoverVolCtrlWidget->ShowAt(pt.rx(), pt.ry());
}

void OBSBasic::on_selectCamera_clicked()
{
    AddSourceWithProperty(SOURCE_DSHOW_INPUT);
}

void OBSBasic::on_selectWindow_clicked()
{
#if defined(_WIN32)
    /*if (!IsAeroEnabled())
    {
        QMessageBox msgBox(this);
        msgBox.setText(QApplication::translate("OBSBasic", "TBLive.Aero.Disabled", 0));
        msgBox.addButton(QTStr("ok"), QMessageBox::AcceptRole);
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setWindowTitle(QTStr("TBLive.MsgTip"));
        msgBox.exec();
    }*/
#endif

    AddSourceWithProperty(SOURCE_WINDOW_CAPTURE);
}

void OBSBasic::on_startStreamBtn_clicked()
{
    if (!outputHandler->StreamingActive()) {
        bool confirm = config_get_bool(GetGlobalConfig(), "BasicWindow",
                                       "WarnBeforeStartingStream");

        if (confirm && isVisible()) {
            QMessageBox::StandardButton button =
            OBSMessageBox::question(this,
                                    QTStr("ConfirmStart.Title"),
                                    QTStr("ConfirmStart.Text"));
            
            if (button == QMessageBox::No)
                return;
        }

		if (!App()->rtmpUrl.empty()) {
			SetStreamURL(App()->rtmpUrl.c_str());
            //blog(LOG_INFO, "%s", App()->rtmpUrl.c_str());
        } else {
            //StreamUrlDialog stream_dialog(nullptr);

            //if (stream_dialog.exec() == QDialog::Accepted) {
            //    QString url = stream_dialog.GetStreamUrl();
            //    if(url.isEmpty()) {
            //        return;
            //    }
            //    else {
            //        SetStreamURL(QT_TO_UTF8(url));
            //        blog(LOG_INFO, "%s", App()->rtmpUrl.c_str());
            //    }
            //} else {
            //    return;
            //}
            return;
        }
        
        StartStreaming();
    }
}

void OBSBasic::on_stopStreamBtn_clicked()
{
    if (outputHandler->StreamingActive()) {
        bool confirm = config_get_bool(GetGlobalConfig(), "BasicWindow",
                                       "WarnBeforeStoppingStream");

        if (confirm && isVisible()) {
            QMessageBox::StandardButton button =
            OBSMessageBox::question(this,
                                    QTStr("ConfirmStop.Title"),
                                    QTStr("ConfirmStop.Text"));

            if (button == QMessageBox::No)
                return;
        }

        StopStreaming();

        //on_action_end_triggered();
    }
}

void OBSBasic::on_selectResolution_clicked()
{
    if (outputHandler->StreamingActive()) {

        OBSMessageBox::information(this, QTStr(""),
                                   QTStr("ConfirmStarted.Text"));

        return;
    }

    QPoint pt = QCursor::pos();

    //悬浮在鼠标上方的位置
    pt.setY(pt.ry() - ui->volCtrlBtn->height() / 2);
    m_pReslSelWidget->ShowAt(pt.rx(), pt.ry());
}

void OBSBasic::on_elementsBtn_clicked()
{
//    if (!elementsDialog) {
//        elementsDialog = new ElementsDialog(this);
//        elementsDialog->setAttribute(Qt::WA_DeleteOnClose, true);
//        QObject::connect(elementsDialog, &ElementsDialog::requestLoginAgain, [this]{
//            blog(LOG_INFO, "Login again because of the timeout of login when config elements animations.");
//            CloseElementDialog();
//            App()->createAndShowLoginWindow(false);
//        });
//
//        //QObject::connect(App()->loginWindow, &TBLiveStudioLoginWindow::loginSucceed, elementsDialog, &ElementsDialog::reloadCurrentPage);
//        //elementsDialog->exec();
//        elementsDialog->show();
//        SystemTray(false);
//        return;
//    }
//
//    if (elementsDialog->isVisible()) {
//        elementsDialog->hide();
//    } else {
//        elementsDialog->show();
//    }
}

void OBSBasic::on_enterLivePlatformBtn_clicked()
{
//    if (outputHandler->StreamingActive()) {
//
//        OBSMessageBox::information(this,
//            QTStr(""), QTStr("ConfirmStarted.Text"));
//
//            return;
//    }

	QDesktopServices::openUrl(QUrl(TBLiveEngine::Instance()->GetUrl(TBLSDK_LIVE_PLATFORM_URL)));
}

void OBSBasic::on_beautifyBtn_clicked()
{
    if (m_faceBeautyEnabled) {
        m_faceBeautyEnabled = false;
        obs_reset_face_beauty_enable(false);

        ui->beautifyBtn->setToolTip(QTStr("FaceBeautify.Closed"));
        ui->beautifyBtn->setStyleSheet("QPushButton { \
                                       background-image:url(:/res/icons/face.png); \
                                       background-position: center; \
                                       background-repeat: no-repeat; \
                                       background-color: transparent; \
                                       }");
    } else {
        m_faceBeautyEnabled = true;
        obs_reset_face_beauty_enable(true);

        ui->beautifyBtn->setToolTip(QTStr("FaceBeautify.Opened"));
        ui->beautifyBtn->setStyleSheet("QPushButton { \
                                       background-image:url(:/res/icons/face-hover.png); \
                                       background-position: center; \
                                       background-repeat: no-repeat; \
                                       background-color: transparent; \
                                       }");
    }
}

void OBSBasic::AddSourceWithProperty(const char *id)
{
    const char *src_name = obs_source_get_display_name(id);
    OBSScene scene = GetCurrentScene();

    if (!scene)
        return;

    //if (strcmp(id, SOURCE_DSHOW_INPUT) == 0
    //    && !obs_scene_has_free_capture_device(scene)) {
    //    OBSMessageBox::information(this,
    //        QTStr("DeviceInUsed.Title"),
    //        QTStr("DeviceInUsed.Text"));
    //    return;
    //}

    obs_source_t * source = nullptr;
    QString placeHolderText{ QT_UTF8(src_name) };

    QString text{ placeHolderText };
    int i = 2;
    while ((source = obs_get_source_by_name(QT_TO_UTF8(text)))) {
        obs_source_release(source);
        text = QString("%1 %2").arg(placeHolderText).arg(i++);
    }

    source = obs_source_create(id, QT_TO_UTF8(text), NULL, nullptr);
    if (source) {
        AddSourceData data;
        data.source = source;
        data.visible = true;

        auto addSource = [](void *_data, obs_scene_t *scene) {
            AddSourceData *data = (AddSourceData *)_data;
            obs_sceneitem_t *sceneitem;
            sceneitem = obs_scene_add(scene, data->source);
            obs_sceneitem_set_visible(sceneitem, data->visible);
        };

        obs_enter_graphics();
        obs_scene_atomic_update(scene, addSource, &data);
        obs_leave_graphics();
    }

    CreatePropertiesWindow(source);
    obs_source_release(source);
}

bool OBSBasic::AddSourcewithURL(const char *id, const char *url)
{
    OBSScene scene = GetCurrentScene();
    bool success = false;

    if (!scene)
        return false;

    obs_source_t * source = nullptr;
    bool exist = false;

    if (strcmp(id, SOURCE_DSHOW_INPUT) == 0) {
        exist = obs_scene_exist_source_with_id(scene, id);
    }

    if (exist) {
        return true;
    } else {
        const char *src_name = obs_source_get_display_name(id);
        OBSScene scene = GetCurrentScene();

        if (!scene || !src_name)
            return false;

        QString placeHolderText{QT_UTF8(src_name)};
        QString text{placeHolderText};
        int i = 2;

        while ((source = obs_get_source_by_name(QT_TO_UTF8(text)))) {
            obs_source_release(source);
            text = QString("%1 %2").arg(placeHolderText).arg(i++);
        }

        source = obs_source_create(id, QT_TO_UTF8(text), NULL, nullptr);
		ConfigSourceIfNeeded(source, url);

        if (source) {
            AddSourceData data;
            data.source = source;
            data.visible = true;
			
			auto addSource = [](void *_data, obs_scene_t *scene) {
                AddSourceData *data = (AddSourceData *)_data;
                obs_sceneitem_t *sceneitem;
                sceneitem = obs_scene_add(scene, data->source);
                obs_sceneitem_set_visible(sceneitem, data->visible);
            };

            obs_enter_graphics();
            obs_scene_atomic_update(scene, addSource, &data);
            obs_leave_graphics();
			
            success = true;
        }
    }

    UpdateNewSource(source, id, url);

    return success;
}

bool OBSBasic::AddElementSource(const char *id, const char* name, const char *url)
{
    OBSScene scene = GetCurrentScene();
    obs_source_t * source = nullptr;
    bool item_add = false;
    bool success = false;

    if (!scene)
        return false;

    if (!id || !name || !url)
        return false;

    source = obs_get_source_by_name(name);
    if (source) {
        obs_sceneitem_t *sceneitem = obs_sceneitem_from_source(scene, source);
        obs_sceneitem_set_visible(sceneitem, true);
        item_add = true;
    } else {
        source = obs_source_create(id, name, NULL, nullptr);
        item_add = false;
    }

    ConfigSourceIfNeeded(source, url);

    if (source && !item_add) {
        AddSourceData data;
        data.source = source;
        data.visible = true;

        auto addSource = [](void *_data, obs_scene_t *scene) {
            AddSourceData *data = (AddSourceData *)_data;
            obs_sceneitem_t *sceneitem;
            sceneitem = obs_scene_add(scene, data->source);
            obs_sceneitem_set_visible(sceneitem, data->visible);
        };

        obs_enter_graphics();
        obs_scene_atomic_update(scene, addSource, &data);
        obs_leave_graphics();
        success = true;
    }


    OBSData settings = obs_source_get_settings(source);
    obs_properties_t *properties = obs_source_properties(source);
    obs_property_t *property = obs_properties_first(properties);
    while (property) {
        const char *property_name = obs_property_name(property);
        if (strcmp(property_name, "file") == 0
            && strcmp(id, SOURCE_IMAGE) == 0) {
            obs_data_set_string(settings, property_name, url);
            break;
        }

        if (strcmp(property_name, "local_file") == 0
            && strcmp(id, SOURCE_FFMPEG) == 0) {
            obs_data_set_string(settings, property_name, url);
            break;
        }

        obs_property_next(&property);
    }

    obs_source_update(source, settings);

    obs_properties_destroy(properties);
    obs_data_release(settings);
    obs_source_release(source);

    return success;
}

void OBSBasic::UpdateNewSource(obs_source_t * source, const char *id, const char *url)
{
    OBSData settings = obs_source_get_settings(source);
    obs_properties_t *properties = obs_source_properties(source);
    obs_property_t *property = obs_properties_first(properties);
    while (property) {
        const char *property_name = obs_property_name(property);
        if (strcmp(property_name, "file") == 0
            && strcmp(id, SOURCE_IMAGE) == 0) {
            obs_data_set_string(settings, property_name, url);
            //obs_source_update(source, settings);
            goto END;
        }

		if (strcmp(property_name, "local_file") == 0
			&& strcmp(id, SOURCE_FFMPEG) == 0) {
			obs_data_set_string(settings, property_name, url);
			//obs_source_update(source, settings);
			goto END;
		}

        size_t count = obs_property_list_item_count(property);
        for (size_t i = 0; i < count; i++) {
            const char *item = obs_property_list_item_name(property, i);

            if (strcmp(property_name, VIDEO_DEVICE_ID) == 0
                && strcmp(item, "") != 0) {
                obs_data_set_string(settings, property_name, obs_property_list_item_string(property, i));
                //obs_source_update(source, settings);  //enable camera caputure flag
                goto END;
            }
        }
        obs_property_next(&property);
    }

END:
    obs_source_update(source, settings);

    obs_properties_destroy(properties);
    obs_data_release(settings);
    obs_source_release(source);

    return;
}

void OBSBasic::SetStreamURL(const char* url)
{
    if (url == NULL || strlen(url) < sizeof("rtmp://")) {
        return;
    }

    obs_service_t *service = GetService();
    const char* id = obs_service_get_id(service);

    obs_data_t *hotkeyData = obs_hotkeys_save_service(service);
    obs_data_t *settings = obs_service_get_settings(service);

    obs_data_set_string(settings, "stream_url", url);
    obs_service_t *newService = obs_service_create(id, "default_service", settings, hotkeyData);

    obs_data_release(settings);
    obs_data_release(hotkeyData);

    if (!newService) {
        return;
    }

    SetService(newService);
    SaveService();
    obs_service_release(newService);

    return;
}

void OBSBasic::SetStreamParams()
{
    ResolutionConfig *config = nullptr;

    if (!App()->needLogin) {
        return;
    }

    if (m_is720p) {
        config = &App()->mConfig.m720pConfig;
    }else {
        config = &App()->mConfig.m360pConfig;
    }


    if (m_isLandscape) {
        config_set_uint(basicConfig, "Video", "BaseCX", config->mBaseCX);
        config_set_uint(basicConfig, "Video", "BaseCY", config->mBaseCY);
        config_set_uint(basicConfig, "Video", "OutputCX", config->mOutputCX);
        config_set_uint(basicConfig, "Video", "OutputCY", config->mOutputCY);
    } else {
        config_set_uint(basicConfig, "Video", "BaseCX", config->mBaseCY);
        config_set_uint(basicConfig, "Video", "BaseCY", config->mBaseCX);
        config_set_uint(basicConfig, "Video", "OutputCX", config->mOutputCY);
        config_set_uint(basicConfig, "Video", "OutputCY", config->mOutputCX);
    }

    config_set_default_uint(basicConfig, "SimpleOutput", "ABitrate", config->mABitrate);
    config_set_default_uint(basicConfig, "Audio", "SampleRate", config->mSampleRate);
    config_set_default_string(basicConfig, "Audio", "ChannelSetup", config->mChannelSetup.c_str());

    config_set_default_uint(basicConfig, "Video", "FPSType", config->mFPSType);
    config_set_default_uint(basicConfig, "Video", "FPSInt", config->mFPSInt);
    config_set_default_uint(basicConfig, "SimpleOutput", "VBitrate", config->mVBitrate);

    ResetAudio();
}

void OBSBasic::UpdataResolutionIcon()
{
    if (m_is720p) {
        ui->selectResolution->setStyleSheet(
            "QToolButton {background-image:url(:/res/icons/720p.png); background-position: center;background-repeat: no-repeat;}"
            "QToolButton:hover { background-image:url(:/res/icons/720p-hover.png); background-position:center;background-repeat: no-repeat; }");
    } else {
        ui->selectResolution->setStyleSheet(
            "QToolButton {background-image:url(:/res/icons/360p.png); background-position: center;background-repeat: no-repeat;}"
            "QToolButton:hover { background-image:url(:/res/icons/360p-hover.png); background-position:center;background-repeat: no-repeat; }");
    }
}

void OBSBasic::on_action_end_triggered()
{
    m_pWindEnd.reset();
    m_pWindEnd.reset(new OBSBasicEnd(NULL));
    m_pWindEnd->SetOBSBasic(this);
    m_pWindEnd->show();
    this->hide();
}

void OBSBasic::SetCustomBasicConfig(bool is_720p)
{
    if (!App()->needLogin) {
        return;
    }

    m_is720p = is_720p;
    m_isLandscape =  App()->videoLandscape;

    blog(LOG_INFO, "SetCustomBasicConfig: m_is720p(%d), m_isLandscape(%d)",
         m_is720p, m_isLandscape);

    config_set_default_string(basicConfig, "Output", "Mode", App()->mConfig.mMode.c_str());
    config_set_default_string(basicConfig, "SimpleOutput", "StreamEncoder", App()->mConfig.mStreamEncoder.c_str());
    config_set_default_bool(basicConfig, "SimpleOutput", "UseAdvanced", App()->mConfig.mUseAdvanced);
    config_set_default_string(basicConfig, "SimpleOutput", "Preset", App()->mConfig.mPreset.c_str());
    config_set_default_string(basicConfig, "Video", "ColorFormat", App()->mConfig.mColorFormat.c_str());
    config_set_default_string(basicConfig, "Video", "ColorSpace", App()->mConfig.mColorSpace.c_str());

    SetStreamParams();
}

void OBSBasic::UploadMonitorInfo() {
	monitorInfoUploadCount++;

	if (monitorInfoUploadCount && (monitorInfoUploadCount % 3 == 0)) {
		QJsonObject json;
		json.insert("userId", QString::fromStdString(App()->userId));
		json.insert("liveId", QString::fromStdString(App()->liveId));
		json.insert("platform", QString::fromStdString(platform));
		json.insert("MonitorPoint", "liveState");
		json.insert("activeFPS", QString::number(obs_get_active_fps()));
		json.insert("bandwidth", QString::number(outputBandwidth));
		json.insert("version", QString(TBLiveEngine::Instance()->GetVerString()));
        App()->mWebProxy->uploadUserInfo(TBUserInfoType::MonitorInfo, json);
	}

	if (monitorInfoUploadCount && (monitorInfoUploadCount % 300 == 0)) {

		// 网络卡顿定义为rtmp缓冲超过一秒且当前发送带宽不大于预设码率的10%，5分钟统计一次
		bool streamCongest = obs_output_get_stream_congest(outputHandler->streamOutput);
		uint32_t videoBitrate = config_get_default_uint(basicConfig, "SimpleOutput", "VBitrate");
		uint32_t audioBitrate = config_get_default_uint(basicConfig, "SimpleOutput", "ABitrate");
		if (streamCongest && outputBandwidth < (videoBitrate + audioBitrate) / 10) {
			QJsonObject json;
			json.insert("userId", QString::fromStdString(App()->userId));
			json.insert("liveId", QString::fromStdString(App()->liveId));
			json.insert("platform", QString::fromStdString(platform));
			json.insert("MonitorPoint", "congest");
			json.insert("bandwidth", QString::number(outputBandwidth));
			json.insert("version", QString(TBLiveEngine::Instance()->GetVerString()));
			App()->mWebProxy->uploadUserInfo(TBUserInfoType::MonitorInfo, json);
		}

		// 检查是否有新的摄像头，如果有则上传它的名字
		CheckIfNewVideoDeviceAdded();
		if (!cameras.empty()) {
			std::for_each(cameras.begin(), cameras.end(), [this](decltype(*cameras.begin()) it){
				if (it.second) {
					QJsonObject json;
					json.insert("userId", QString::fromStdString(App()->userId));
					json.insert("liveId", QString::fromStdString(App()->liveId));
					json.insert("platform", QString::fromStdString(platform));
					json.insert("MonitorPoint", "CameraName");
					json.insert("camera", QString::fromWCharArray(it.first.c_str()));
					json.insert("version", QString(TBLiveEngine::Instance()->GetVerString()));
					App()->mWebProxy->uploadUserInfo(TBUserInfoType::MonitorInfo, json);
				}
			});
		}

		monitorInfoUploadCount = 0;
	}
}

void OBSBasic::GetCurrentVideoCaptureFPS() {
	auto func = [](obs_scene_t * scene, obs_sceneitem_t* item, void *param)
	{
			std::map<std::string, int16_t> &it =
				*reinterpret_cast<std::map<std::string, int16_t>*>(param);
			if (obs_scene_items_with_id(item, SOURCE_DSHOW_INPUT)) {
				//obs_source_t* source = obs_sceneitem_get_source(item);
				//obs_data_t *settings = obs_source_get_settings(source);
				//std::string video_device_id = obs_data_get_string(settings, "video_device_id");
				//int16_t captureFPS = obs_source_get_capture_fps(source);
				//it.emplace(video_device_id, captureFPS);
			}
			return true;
	};

	currentVideoCaptureFPS.clear();
	obs_scene_enum_items(GetCurrentScene(), func, &currentVideoCaptureFPS);
}

void OBSBasic::UploadStreamTimeDuration() {
	if (totalStreamSeconds > 0) {
		QJsonObject json;
		json.insert("userId", QString::fromStdString(App()->userId));
		json.insert("liveId", QString::fromStdString(App()->liveId));
		json.insert("platform", QString::fromStdString(platform));
		json.insert("MonitorPoint", "liveTime");
		json.insert("liveSeconds", QString::number(totalStreamSeconds, 10));
		json.insert("version", QString(TBLiveEngine::Instance()->GetVerString()));
		App()->mWebProxy->uploadUserInfo(TBUserInfoType::MonitorInfo, json);
		
		totalStreamSeconds = 0;
	}
}

void OBSBasic::UploadTBLiveLog(const char *file)
{
    BPtr<char> fileString{ ReadLogFile(file) };

    if (!fileString)
        return;

    if (!*fileString)
        return;

    ui->menuLogFiles->setEnabled(false);

    QJsonObject json;
    json.insert("filename", QString::fromStdString(file));
    json.insert("file", QString::fromStdString(fileString.Get()));

    App()->mWebProxy->uploadUserInfo(TBUserInfoType::UserLogs, json);

    ui->menuLogFiles->setEnabled(true);
}

void OBSBasic::UploadSelectedLog()
{
    char logDir[512];
    if (GetConfigPath(logDir, sizeof(logDir), "TBLiveStudio/logs") <= 0)
        return;

    QString filter;
    QString path = QFileDialog::getOpenFileName(
        this, QTStr("Browse"),
        logDir, filter);

    BPtr<char> fileString = os_quick_read_utf8_file(QT_TO_UTF8(path));
    if (!fileString)
        blog(LOG_WARNING, "Failed to read log file %s", QT_TO_UTF8(path));

    if (!fileString)
        return;

    if (!*fileString)
        return;

    ui->menuLogFiles->setEnabled(false);

    QJsonObject json;
    QString filename = "TBLiveStudio/logs/UserID_";
    filename += App()->userId.c_str();
    filename += "_";
    filename += path.mid(strlen(logDir) + 1,
        path.length() - strlen(logDir) - 1);

    json.insert("filename", filename);
    json.insert("file", QString::fromStdString(fileString.Get()));

    App()->mWebProxy->uploadUserInfo(TBUserInfoType::UserLogs, json);

    ui->menuLogFiles->setEnabled(true);
}


void OBSBasic::ConfigSourceIfNeeded(obs_source_t * source, const char* url) {
	if (!source || !url) {
		return;
	}

	obs_data* settings = obs_source_get_settings(source);
	if (settings) {
		std::string type_value;
		QUrl qurl(QString::fromLocal8Bit(url));
		QUrlQuery urlQuery(qurl);
		if (urlQuery.hasQueryItem("type")) {
			type_value = urlQuery.queryItemValue("type").toStdString();
		}
		if (type_value.compare(TP_MODEL) == 0) {
			obs_data_set_bool(settings, "looping", false);
			obs_data_set_int(settings, "element_type", TBLive_MODEL);
		}
		else if (type_value.compare(TP_COUNTDOWN) == 0) {
			obs_data_set_bool(settings, "looping", false);
			obs_data_set_int(settings, "element_type", TBLive_COUNTDOWN);
		}
		else if (type_value.compare(TP_AVINTERACTIVE) == 0) {
			obs_data_set_bool(settings, "looping", false);
			obs_data_set_int(settings, "element_type", TBLive_AVINTERACTIVE);
		}
        else if (type_value.compare(MARQUEE) == 0) {
            obs_data_set_bool(settings, "looping", true);
            obs_data_set_int(settings, "element_type",TBLive_MARQUEE);
        }
		obs_data_release(settings);
	}
}

void OBSBasic::SourceRemoveCallback(void *data, calldata_t *params)
{
    obs_source_t *source = (obs_source_t*)calldata_ptr(params, "source");

    QMetaObject::invokeMethod(static_cast<OBSBasic*>(data),
        "RemoveItemFromSource",
        Q_ARG(OBSSource, OBSSource(source)));
}

void OBSBasic::SourceStartRendering(void *data, calldata_t *params){
	if (!data || !params) {
		return;
	}

	obs_source_t *source = (obs_source_t*)calldata_ptr(params, "source");
	const char* id = obs_source_get_id(source);
	if (strcmp(id, SOURCE_IMAGE) == 0) {
		obs_data_t* settings = obs_source_get_settings(source);
		if (settings) {
			int element_type = obs_data_get_int(settings, "element_type");
			if (1 == element_type || 2 == element_type) {
				QMetaObject::invokeMethod(static_cast<OBSBasic*>(data),
					"ResetElementImageSourcePosition",
					Q_ARG(OBSSource, OBSSource(source)));
			}
		}
	}
}

void OBSBasic::RemoveItemFromSource(OBSSource source)
{
	if (!source) {
		return;
	}
    int element_type = -1;
	obs_data_t* settings = obs_source_get_settings(source);
	if (settings) {
		element_type = obs_data_get_int(settings, "element_type");
		const char* source_id = obs_source_get_id(source);
        if (elementView) {
            if (TBLive_MODEL == element_type) {
                elementView->elementsRenderedDone(TP_MODEL, source_id);
            }
            else if (TBLive_AVINTERACTIVE == element_type) {
                elementView->elementsRenderedDone(TP_AVINTERACTIVE, source_id);
            }
            else if (TBLive_COUNTDOWN == element_type) {
                elementView->elementsRenderedDone(TP_COUNTDOWN, source_id);
            }
            else if (TBLive_MARQUEE == element_type) {
                elementView->elementsRenderedDone(MARQUEE, source_id);
            }
        }
		obs_data_release(settings);
	}

    OBSScene scene = GetCurrentScene();
    obs_sceneitem_t *item = obs_sceneitem_from_source(scene, source);

    if (item){
        //obs_sceneitem_remove(item);
        if (element_type != 0)
            obs_sceneitem_set_visible(item, false);
    }
}

void OBSBasic::CloseElementDialog() {
//    if (elementsDialog) {
//        elementsDialog->close();
//    }
}

void OBSBasic::ConfigVideoEncoderParam() {
    if(outputHandler) {
        outputHandler->ConfigVideoEncoderParam();
    }
}

void OBSBasic::UploadStreamInfo() {
    
    QJsonObject api, data;
    data.insert("liveId", QString::fromStdString(App()->liveId));
	std::string pushFeature = platform;
	pushFeature.append("_").append("rtmp").append("_").append((m_is720p ? "720" : "360"));
    data.insert("pushFeature", QString::fromStdString(pushFeature));
    api.insert("api", "mtop.mediaplatform.live.updatePushFeature");
    api.insert("v", "1.0");
    api.insert("data", data);
	App()->mWebProxy->tbliveBusinessRequest("mtop", api, [](QString value, QString status){
		if (!status.isEmpty()) {
			blog(LOG_INFO, "OBSBasic::UploadStreamInfo() %s", status.toStdString().c_str());
		}
	});
}

void OBSBasic::CheckIfNewVideoDeviceAdded() {
	auto func = [](obs_scene_t * scene, obs_sceneitem_t* item, void *param)
	{
		std::map<std::wstring, bool> &devices =
			*reinterpret_cast<std::map<std::wstring, bool>*>(param);
		if (obs_scene_items_with_id(item, SOURCE_DSHOW_INPUT)) {
			obs_source_t* source = obs_sceneitem_get_source(item);
			if (source) {
				const wchar_t* camera_name = obs_get_camera_name(source);
				if (camera_name) {
					if (devices.find(camera_name) != devices.end()) {
						devices.at(camera_name) = false;
					}
					else {
						devices.emplace(camera_name, true);
					}
				}
			}
		}
		return true;
	};

	obs_scene_enum_items(GetCurrentScene(), func, &cameras);
}
