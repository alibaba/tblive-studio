#include "graphics.h"
#include "image-file.h"

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include "../obs-ffmpeg-compat.h"

typedef struct APNGDemuxContext {
    const AVClass *class;

    int max_fps;
    int default_fps;

    int64_t pkt_pts;
    int pkt_duration;

    int is_key_frame;

    /*
    * loop options
    */
    int ignore_loop;
    uint32_t num_frames;
    uint32_t num_play;
    uint32_t cur_loop;
} APNGDemuxContext;

typedef struct ffmpeg_image {
    const char         *file;
    AVFormatContext    *fmt_ctx;
    AVCodecContext     *decoder_ctx;
    AVCodec            *decoder;
    AVStream           *stream;
    int                stream_idx;

    int                cx, cy;
    enum AVPixelFormat format;

    bool     initialized;
    bool     cached;
}ff_image_t;

static bool ffmpeg_image_open_decoder_context(struct ffmpeg_image *info)
{
	int ret = av_find_best_stream(info->fmt_ctx, AVMEDIA_TYPE_VIDEO,
			-1, 1, NULL, 0);
	if (ret < 0) {
		blog(LOG_WARNING, "Couldn't find video stream in file '%s': %s",
				info->file, av_err2str(ret));
		return false;
	}

	info->stream_idx  = ret;
	info->stream      = info->fmt_ctx->streams[ret];
	info->decoder_ctx = info->stream->codec;
	info->decoder     = avcodec_find_decoder(info->decoder_ctx->codec_id);

	if (!info->decoder) {
		blog(LOG_WARNING, "Failed to find decoder for file '%s'",
				info->file);
		return false;
	}

	ret = avcodec_open2(info->decoder_ctx, info->decoder, NULL);
	if (ret < 0) {
		blog(LOG_WARNING, "Failed to open video codec for file '%s': "
		                  "%s", info->file, av_err2str(ret));
		return false;
	}

	return true;
}

static void ffmpeg_image_free(struct ffmpeg_image *info)
{
    if (info->initialized){
        info->initialized = false;

        avcodec_close(info->decoder_ctx);
        avformat_close_input(&info->fmt_ctx);
    }
}

static bool ffmpeg_image_init(struct ffmpeg_image *info, const char *file)
{
	int ret;

	if (!file || !*file)
		return false;

    if (info->initialized)
        return false;

	memset(info, 0, sizeof(struct ffmpeg_image));
	info->file       = file;
	info->stream_idx = -1;
    info->cached     = true;

    avformat_network_init();
	ret = avformat_open_input(&info->fmt_ctx, file, NULL, NULL);
	if (ret < 0) {
		blog(LOG_WARNING, "Failed to open file '%s': %s",
				info->file, av_err2str(ret));
		return false;
	}

	ret = avformat_find_stream_info(info->fmt_ctx, NULL);
	if (ret < 0) {
		blog(LOG_WARNING, "Could not find stream info for file '%s':"
		                  " %s", info->file, av_err2str(ret));
		goto fail;
	}

	if (!ffmpeg_image_open_decoder_context(info))
		goto fail;

    info->initialized = true;

	info->cx     = info->decoder_ctx->width;
	info->cy     = info->decoder_ctx->height;
	info->format = info->decoder_ctx->pix_fmt;

	return true;

fail:
	ffmpeg_image_free(info);
	return false;
}

static bool ffmpeg_image_reformat_frame(struct ffmpeg_image *info,
		AVFrame *frame, uint8_t *out, int linesize)
{
	struct SwsContext *sws_ctx = NULL;
	int               ret      = 0;
    
    // 依据实际上frame的format来做格式转换  by weihe
    if (info->format != frame->format) {
        info->format = frame->format;
    }

	if (frame->format != info->format) {
		info->format = frame->format;
	}

	if (info->format == AV_PIX_FMT_RGBA ||
	    info->format == AV_PIX_FMT_BGRA ||
	    info->format == AV_PIX_FMT_BGR0) {

		if (linesize != frame->linesize[0]) {
			int min_line = linesize < frame->linesize[0] ?
				linesize : frame->linesize[0];

			for (int y = 0; y < info->cy; y++)
				memcpy(out + y * linesize,
				       frame->data[0] + y * frame->linesize[0],
				       min_line);
		} else {
			memcpy(out, frame->data[0], linesize * info->cy);
		}

	} else {
		sws_ctx = sws_getContext(info->cx, info->cy, info->format,
				info->cx, info->cy, AV_PIX_FMT_BGRA,
				SWS_POINT, NULL, NULL, NULL);
		if (!sws_ctx) {
			blog(LOG_WARNING, "Failed to create scale context "
			                  "for '%s'", info->file);
			return false;
		}

		ret = sws_scale(sws_ctx, (const uint8_t *const*)frame->data,
				frame->linesize, 0, info->cy, &out, &linesize);
		sws_freeContext(sws_ctx);

		if (ret < 0) {
			blog(LOG_WARNING, "sws_scale failed for '%s': %s",
					info->file, av_err2str(ret));
			return false;
		}

		info->format = AV_PIX_FMT_BGRA;
	}

	return true;
}

static int ffmpeg_image_decode(struct ffmpeg_image *info, uint8_t *out,
		int linesize)
{
	AVPacket          packet    = {0};
	bool              success   = false;
	AVFrame           *frame    = av_frame_alloc();
	int               got_frame = 0;
    int               pkt_duration = 0;
	int               ret;

	if (!frame) {
		blog(LOG_WARNING, "Failed to create frame data for '%s'",
				info->file);
		return false;
	}

    ret = av_read_frame(info->fmt_ctx, &packet);
	if (ret < 0) {
		blog(LOG_WARNING, "Failed to read image frame from '%s': %s",
				info->file, av_err2str(ret));
		goto fail;
	}

	while (!got_frame) {
		ret = avcodec_decode_video2(info->decoder_ctx, frame,
				&got_frame, &packet);
		if (ret < 0) {
			blog(LOG_WARNING, "Failed to decode frame for '%s': %s",
					info->file, av_err2str(ret));
			goto fail;
		}
	}

	success = ffmpeg_image_reformat_frame(info, frame, out, linesize);

    pkt_duration = frame->pkt_duration;

fail:
	av_free_packet(&packet);
	av_frame_free(&frame);
	return pkt_duration;
}

void gs_init_image_deps(void)
{
	av_register_all();
}

void gs_free_image_deps(void)
{
}

static inline enum gs_color_format convert_format(enum AVPixelFormat format)
{
	switch ((int)format) {
	case AV_PIX_FMT_RGBA: return GS_RGBA;
	case AV_PIX_FMT_BGRA: return GS_BGRA;
	case AV_PIX_FMT_BGR0: return GS_BGRX;
	}

	return GS_BGRX;
}

uint8_t *gs_create_texture_file_data(const char *file,
		enum gs_color_format *format,
		uint32_t *cx_out, uint32_t *cy_out)
{
	struct ffmpeg_image image;
	uint8_t *data = NULL;

	if (ffmpeg_image_init(&image, file)) {
		data = bmalloc(image.cx * image.cy * 4);

		if (ffmpeg_image_decode(&image, data, image.cx * 4)) {
			*format = convert_format(image.format);
			*cx_out = (uint32_t)image.cx;
			*cy_out = (uint32_t)image.cy;
		} else {
			bfree(data);
			data = NULL;
		}

		ffmpeg_image_free(&image);
	}

	return data;
}
bool gs_process_animated_image(gs_image_file_t *image, ff_image_t *ff_image)
{
    uint32_t buffer_num = 0;
    uint32_t frame_size = 0;
    int32_t  pkt_duration;

    bool success = true;

    APNGDemuxContext *ctx = ff_image->fmt_ctx->priv_data;
    image->loops = ctx->num_play;
    image->num_frames = ctx->num_frames;

    image->cached = ff_image->cached;


    buffer_num = ff_image->cached ? image->num_frames : 1;
    frame_size = ff_image->cx * ff_image->cy * 4;

    image->animation_frame_data  = bzalloc(frame_size * buffer_num);
    image->apng_duration         = bzalloc(image->num_frames * sizeof(uint64_t));
    image->animation_frame_cache = bzalloc(image->num_frames*sizeof(uint8_t **));

    memset(image->apng_duration, 0,
           image->num_frames * sizeof(uint64_t));
    memset(image->animation_frame_cache, 0,
           image->num_frames * sizeof(uint8_t *));

    if (ff_image->cached) {
        image->animation_frame_cache[0] = image->animation_frame_data;

        pkt_duration = ffmpeg_image_decode(ff_image,
                                      image->animation_frame_cache[0],
                                      ff_image->cx * 4);
        if (pkt_duration) {
            image->format = convert_format(ff_image->format);
            image->cx = (uint32_t)ff_image->cx;
            image->cy = (uint32_t)ff_image->cy;

            image->apng_duration[0] = pkt_duration
                * ((ff_image->fmt_ctx->streams[0]->time_base.num * 1000000000ULL)
                / ff_image->fmt_ctx->streams[0]->time_base.den);
        }
    }

    return success;
}

uint8_t *gs_create_texture_file_data_animated(const char *file, void *ptr)
{
    gs_image_file_t *image = ptr;
    ff_image_t *ff_image = bmalloc(sizeof(ff_image_t));
    APNGDemuxContext *ctx = NULL;
    uint8_t *data = NULL;
    bool success = false;

    memset(ff_image, 0, sizeof(ff_image_t));

    if (ffmpeg_image_init(ff_image, file)) {
        image->format = convert_format(ff_image->format);
        image->cx = (uint32_t)ff_image->cx;
        image->cy = (uint32_t)ff_image->cy;
        image->ff_image = ff_image;
        image->play_finished = false;

        data = bmalloc(ff_image->cx * ff_image->cy * 4);

        if (strcmp(ff_image->fmt_ctx->iformat->name, "apng") == 0){
            ctx = ff_image->fmt_ctx->priv_data;

            if (ctx->num_frames > 1) {
                image->is_animated = true;
                image->ani_type = APNG_ANIMATION_TYPE;
            }
        }

        if (image->is_animated){
            success = gs_process_animated_image(image, ff_image);
        } else {
            success = ffmpeg_image_decode(ff_image, data, ff_image->cx * 4);
            image->format = convert_format(ff_image->format);
            image->cx = (uint32_t)ff_image->cx;
            image->cy = (uint32_t)ff_image->cy;
            image->cached = false;

            gs_free_ffmpeg_image(ptr);
        }

        if (!success) {
            bfree(data);
            data = NULL;
        }

    } else {
        bfree(ff_image);
    }

    return data;
}

void gs_free_ffmpeg_image(void *ptr)
{
    gs_image_file_t *image = ptr;
    if (image->ff_image){
        ffmpeg_image_free(image->ff_image);
        bfree(image->ff_image);
        image->ff_image = NULL;
    }
}

void gs_decode_ffmpeg_image(void *ptr, uint32_t new_frame)
{
    gs_image_file_t *image = ptr;
    ff_image_t *ff_image = image->ff_image;
    APNGDemuxContext *ctx = NULL;
    uint32_t i;

    if (new_frame > image->num_frames){
        blog(LOG_ERROR, "frame(%d) more than max frame num(%d)",
             new_frame, image->num_frames);
        return;
    }

    if (new_frame < image->cur_frame){
        if(!image->enable_loop){
            image->play_finished = true;
            return;
        }

        ffmpeg_image_free(ff_image);
        ffmpeg_image_init(ff_image, ff_image->file);

        image->animation_frame_cache[image->cur_frame] = NULL;
        image->cur_frame = 0;
    }

    if (new_frame == 0){
        ffmpeg_image_decode(ff_image, image->animation_frame_data, ff_image->cx * 4);

        if (image->apng_duration[0] == 0){
            ctx = ff_image->fmt_ctx->priv_data;
            image->apng_duration[0] = ctx->pkt_duration*
                ((ff_image->fmt_ctx->streams[0]->time_base.num * 1000000000ULL)
                 / ff_image->fmt_ctx->streams[0]->time_base.den);
        }
    }

    for (i = image->cur_frame; i < new_frame; i++){
        ffmpeg_image_decode(ff_image, image->animation_frame_data, ff_image->cx * 4);

        if (image->apng_duration[i+1] == 0) {
            ctx = ff_image->fmt_ctx->priv_data;
            image->apng_duration[i+1] = ctx->pkt_duration*
                ((ff_image->fmt_ctx->streams[0]->time_base.num * 1000000000ULL)
                 / ff_image->fmt_ctx->streams[0]->time_base.den);
        }
    }

    image->animation_frame_cache[image->cur_frame] = NULL;
    image->animation_frame_cache[new_frame] =
        image->animation_frame_data;

    if (image->cur_frame == image->num_frames - 1){
        if (!image->enable_loop){
            image->play_finished = true;
        }
        ffmpeg_image_free(ff_image);
        ffmpeg_image_init(ff_image, ff_image->file);
    }
}


void gs_decode_ffmpeg_image_cached(void *ptr, uint32_t new_frame)
{
    gs_image_file_t *image = ptr;
    ff_image_t *ff_image = image->ff_image;
    APNGDemuxContext *ctx = NULL;
    uint32_t i, frame_size;
    int32_t pkt_duration;

    if (new_frame > image->num_frames){
        blog(LOG_ERROR, "frame(%d) more than max frame num(%d)",
             new_frame, image->num_frames);
        return;
    }

    frame_size = image->cx * image->cy * 4;
    for (i = image->cur_frame + 1; i <= new_frame; i++){
        image->animation_frame_cache[i] = (i-1) * frame_size
            + image->animation_frame_data;

        pkt_duration = ffmpeg_image_decode(ff_image,
                                           image->animation_frame_cache[i],
                                           ff_image->cx * 4);

        ctx = ff_image->fmt_ctx->priv_data;
        image->apng_duration[i] = pkt_duration
            * ((ff_image->fmt_ctx->streams[0]->time_base.num * 1000000000ULL)
            / ff_image->fmt_ctx->streams[0]->time_base.den);

//        blog(LOG_INFO, "image->apng_duration[%d] = %lld, pkt_duration = %d, success = %d",
//             i, image->apng_duration[i], ctx->pkt_duration, success);
    }
}
