#pragma once

// blackmagic 采集卡
#define SOURCE_OBS_BMC_CAPTURE            "decklink-input"

// 媒体源
#define SOURCE_FFMPEG                   "ffmpeg_source"

// 图像源
#define SOURCE_IMAGE                    "image_source"

// 窗口捕获
#define SOURCE_WINDOW_CAPTURE           "window_capture"

#if defined(_WIN32)

// 摄像头
#define SOURCE_DSHOW_INPUT                "dshow_input"

// 音频输入（麦克风）
#define SOURCE_WASAPI_INPUT_CAPTURE        "wasapi_input_capture"

// 音频输出（电脑输出设备，如听筒，用于捕捉如游戏，播放音乐等）
#define SOURCE_WASAPI_OUTPUT_CAPTURE    "wasapi_output_capture"

// 桌面捕获
#define SOURCE_MONITOR_CAPTURE          "monitor_capture"

// 文字源
#define SOURCE_TEXT                     "text_gdiplus"

#define VIDEO_DEVICE_ID                 "video_device_id"


#else

#define SOURCE_DSHOW_INPUT                "av_capture_input"

#define SOURCE_WASAPI_INPUT_CAPTURE        "coreaudio_input_capture"

#define SOURCE_WASAPI_OUTPUT_CAPTURE    "coreaudio_output_capture"

#define SOURCE_MONITOR_CAPTURE          "display_capture"

#define SOURCE_TEXT                        "text_ft2_source"

#define VIDEO_DEVICE_ID                 "device"

#endif

