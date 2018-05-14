#define FACE_AR_ENGINE_API

#include <iostream>
#include <string>
#include <chrono>
#include "util/base.h"
#include "FaceAREngine.h"
#include "face_beauty.h"
#include "media-io/video-scaler.h"

unsigned char * LoadDataBuffer(char *filename, int & buf_size)
{
   unsigned char *model_buf = nullptr;
   fpos_t pos;

   FILE *fp;
   fp = fopen(filename, "rb");
   if (!fp)
   {
	   std::string file_path(filename);
	   std::string file_path_next("../");
	   std::string filename_next = file_path_next + file_path;
	   fp = fopen(filename_next.c_str(), "rb");
	   if(!fp)
	   {
		   return NULL;
	   }
   }
   fseek(fp, 0, SEEK_END);
   fgetpos(fp, &pos);
   buf_size = pos;
   model_buf = (unsigned char *) malloc(sizeof(char) * buf_size);
   fseek(fp, 0, SEEK_SET);
   fread(model_buf, sizeof(char), buf_size, fp);
   fclose(fp);

   return model_buf;
}

void SetFaceBeautyParam(CFaceAREngine* faceEng)
{
   SetFaceBeaytifyParam param;
   param.uFlgs = (FACE_BUFFING_BEAUTY); //WHOLE_WHITENING| 
   param.param.fSlimIntensity = 0.1;
   param.param.nBuffingIntensity = 1;
   param.param.nFaceWhitenIntensity = 1;
   param.param.fEnlargeEyeIntensity = 0.1;
   param.param.fPullJawIntensity = 0.0;
   faceEng->SetParameter(&param); 
}

#ifdef __cplusplus
extern "C" {


FBEngine face_beauty_create(enum video_format format_src, int video_width, int video_height, char* face_model_file)
{

	FBEngine fb_engine = new FACE_BEAUTY_ENGINE;
	if(!face_model_file)
	{
		return NULL;
	}
	fb_engine->data = nullptr;
	fb_engine->engine = nullptr;

	int face_buf_size = 0;
	unsigned char *face_model_dat  = LoadDataBuffer(face_model_file, face_buf_size);
	if(!face_model_dat)
	{
		face_beauty_release(fb_engine);
		return nullptr;
	}
	CFaceAREngine* engine = CFaceAREngine::GetInstance();
	if(!engine)
	{
		free(face_model_dat);
		return nullptr;
	}

	engine->Initialize(face_model_dat, face_buf_size, video_width, video_height, PREVIEW_YUV420SPNV12);
	free(face_model_dat);

	SetAlgorithmTypeParam typeParam;
	typeParam.type = CPU_VERSION;
	engine->SetParameter(&typeParam);

	// windows系统下需要设置美颜库旋转180度后再去做人脸检测
//#if defined(_WIN32)
//	SetRotateParam rotateParam;
//	rotateParam.nRotate = 270;
//	engine->SetParameter(&rotateParam);
//#endif

	SetFaceBeautyParam(engine);
	fb_engine->data = new unsigned char[video_width * video_height * 3 / 2];
	fb_engine->engine = (void*)engine;
	fb_engine->width = video_width;
	fb_engine->height = video_height;
	fb_engine->rotate = 0;
	fb_engine->scaler_info_dst.format = VIDEO_FORMAT_NV12;
	fb_engine->scaler_info_dst.width = video_width;
	fb_engine->scaler_info_dst.height = video_height;

	fb_engine->scaler_info_src.format = format_src;
	fb_engine->scaler_info_src.width = video_width;
	fb_engine->scaler_info_src.height = video_height;
	video_scaler_create(&fb_engine->scaler, &fb_engine->scaler_info_dst, &fb_engine->scaler_info_src, VIDEO_SCALE_DEFAULT);

	blog(LOG_INFO, "[face_beauty] set up success size:%d x %d\n", video_width, video_height);
	return fb_engine;
}

void face_beauty_release(FBEngine fb_engine)
{
	if(fb_engine)
	{
		if(fb_engine->engine)
		{
			CFaceAREngine* engine = (CFaceAREngine*)fb_engine->engine;
			CFaceAREngine::ReleaseInstance(engine);
			delete[] fb_engine->data;
			fb_engine->engine = nullptr;
			fb_engine->data = nullptr;
			video_scaler_destroy(fb_engine->scaler);
			fb_engine->scaler = NULL;
		}

		delete fb_engine;
	}
}

bool face_beauty_scaler(FBEngine fb_engine,
		uint8_t *output[], const uint32_t out_linesize[],
		const uint8_t* input[], const uint32_t in_linesize[])
{
	if(!fb_engine)
	{
		return false;
	}
	return video_scaler_scale(fb_engine->scaler, output, out_linesize, input, in_linesize);
}

void face_beauty(FBEngine fb_engine, unsigned char* y_data, unsigned char* uv_data, int video_width, int video_height)
{
	if(!fb_engine || !y_data || !uv_data) 
	{
		return;
	}

	CFaceAREngine* engine = (CFaceAREngine*)fb_engine->engine;

	memcpy(fb_engine->data, y_data, video_width * video_height);
	memcpy(fb_engine->data + video_width * video_height, uv_data, video_width * video_height / 2);

	auto now = std::chrono::steady_clock::now();
	int result = engine->DoVideoData(fb_engine->data, video_width, video_height);
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - now);
	if (duration < std::chrono::milliseconds(5)) {
		fb_engine->rotate += 90;
		SetRotateParam rotateParam;
		rotateParam.nRotate = (fb_engine->rotate % 360);
		engine->SetParameter(&rotateParam);
	}
	//blog(LOG_DEBUG, "FaceBeauty time used %lld ms with result(%d)", duration.count(), result);

	memcpy(y_data, fb_engine->data, video_width * video_height);
	memcpy(uv_data, fb_engine->data + video_width * video_height, video_width * video_height / 2);
	return;
}

}
#endif


