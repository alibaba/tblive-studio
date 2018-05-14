#ifndef FACE_BEAUTY_H_
#define FACE_BEAUTY_H_

#include "media-io/video-io.h"
#include "media-io/video-scaler.h"

typedef struct FACE_BEAUTY_ENGINE
{
	void* engine;
	unsigned char* data;
	int width;
	int height;

	int rotate;

	video_scaler_t* scaler;
	struct video_scale_info scaler_info_src;
	struct video_scale_info scaler_info_dst;
}FACE_BEAUTY_ENGINE;

typedef struct FACE_BEAUTY_ENGINE* FBEngine;

#ifdef __cplusplus
extern "C" {

	FBEngine face_beauty_create(enum video_format format_src, int video_width, int video_height, char* face_model_file);
	void face_beauty_release(FBEngine fb_engine);
	void face_beauty(FBEngine fb_engine, unsigned char* y_data, unsigned char* uv_data, int video_width, int video_height);

	bool face_beauty_scaler(FBEngine fb_engine,
		uint8_t *output[], const uint32_t out_linesize[],
		const uint8_t * input[], const uint32_t in_linesize[]);
}
#endif

#endif


