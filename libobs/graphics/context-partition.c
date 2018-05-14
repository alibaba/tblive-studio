/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "context-partition.h"
#include "obs.h"
#include "util/platform.h"
#include "stdio.h"

#define CHROMAKEY "chromakey"
#define COLORKEY "colorkey"

static inline enum AVPixelFormat get_ffmpeg_video_format(enum video_format format)
{
	switch (format) {
	case VIDEO_FORMAT_NONE: return AV_PIX_FMT_NONE;
	case VIDEO_FORMAT_I420: return AV_PIX_FMT_YUV420P;
	case VIDEO_FORMAT_NV12: return AV_PIX_FMT_NV12;
	case VIDEO_FORMAT_YVYU: return AV_PIX_FMT_NONE;
	case VIDEO_FORMAT_YUY2: return AV_PIX_FMT_YUYV422;
	case VIDEO_FORMAT_UYVY: return AV_PIX_FMT_UYVY422;
	case VIDEO_FORMAT_RGBA: return AV_PIX_FMT_RGBA;
	case VIDEO_FORMAT_BGRA: return AV_PIX_FMT_BGRA;
	case VIDEO_FORMAT_BGRX: return AV_PIX_FMT_BGRA;
	case VIDEO_FORMAT_Y800: return AV_PIX_FMT_GRAY8;
	case VIDEO_FORMAT_I444: return AV_PIX_FMT_YUV444P;
	}

	return AV_PIX_FMT_NONE;
}

static inline enum video_format convert_pixel_format(int f)
{
	switch (f) {
	case AV_PIX_FMT_NONE:    return VIDEO_FORMAT_NONE;
	case AV_PIX_FMT_YUV420P: return VIDEO_FORMAT_I420;
	case AV_PIX_FMT_NV12:    return VIDEO_FORMAT_NV12;
	case AV_PIX_FMT_YUYV422: return VIDEO_FORMAT_YUY2;
	case AV_PIX_FMT_UYVY422: return VIDEO_FORMAT_UYVY;
	case AV_PIX_FMT_RGBA:    return VIDEO_FORMAT_RGBA;
	case AV_PIX_FMT_BGRA:    return VIDEO_FORMAT_BGRA;
	case AV_PIX_FMT_BGR0:    return VIDEO_FORMAT_BGRX;
	default:;
	}

	return VIDEO_FORMAT_NONE;
}

bool did_video_frame_property_changed(video_partitioner* context, struct obs_source_frame *frame) {
	if (!context || !frame) {
		return true;
	}

	if (frame->width != context->info.width || frame->height != context->info.height || frame->format != context->info.format) {
		return true;
	}

	return false;
}

static video_partitioner* create_context_partition(struct obs_source_frame *frame) {
	video_partitioner* partitioner = (video_partitioner*)bmalloc(sizeof(struct ffmpeg_video_context_partition_t));
	if (!partitioner) {
		blog(LOG_ERROR, "ffmpeg_video_context_partition_t bmalloc failed.");
		return NULL;
	}

	avfilter_register_all();
	partitioner->filter_graph = avfilter_graph_alloc();
	partitioner->info.format = frame->format;
	partitioner->info.width = frame->width;
	partitioner->info.height = frame->height;
	partitioner->format = get_ffmpeg_video_format(frame->format);

	if (format_is_yuv(frame->format)) {
		partitioner->param.type = chroma_key;
		snprintf(partitioner->param.color, sizeof(partitioner->param.color), "0x70de77");
		partitioner->param.similarity = 0.12;
		partitioner->param.blend = 0.05;
	}
	else {
		partitioner->param.type = color_key;
		snprintf(partitioner->param.color, sizeof(partitioner->param.color), "0x00fa00");
		partitioner->param.similarity = 0.73;
		partitioner->param.blend = 0.02;
	}

	partitioner->input_frame = av_frame_alloc();
	partitioner->output_frame = av_frame_alloc();

	snprintf(partitioner->input_filter_config, sizeof(partitioner->input_filter_config), "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d", 
		frame->width, frame->height, partitioner->format, 1, 25, 1, 1);
	int err_code = avfilter_graph_create_filter(&partitioner->filter_buffer_ctx, avfilter_get_by_name("buffer"), 
		"in", partitioner->input_filter_config, NULL, partitioner->filter_graph);
	if (err_code < 0) {
		blog(LOG_ERROR, "avfilter_graph_create_filter buffer failed with error code %d", err_code);
		goto fail;
	}

	snprintf(partitioner->colorspacekey_filter_config, sizeof(partitioner->colorspacekey_filter_config), "color=%s:similarity=%f:blend=%f",
		partitioner->param.color, partitioner->param.similarity, partitioner->param.blend);
	if (partitioner->param.type == chroma_key) {
		err_code = avfilter_graph_create_filter(&partitioner->filter_colorspacekey_ctx, avfilter_get_by_name(CHROMAKEY), CHROMAKEY, partitioner->colorspacekey_filter_config, NULL, partitioner->filter_graph);
	}
	else if (partitioner->param.type == color_key) {
		err_code = avfilter_graph_create_filter(&partitioner->filter_colorspacekey_ctx, avfilter_get_by_name(COLORKEY), COLORKEY, partitioner->colorspacekey_filter_config, NULL, partitioner->filter_graph);
	}
	if (err_code) {
		blog(LOG_ERROR, "avfilter_graph_create_filter colorspace failed with error code %d", err_code);
		goto fail;
	}

	enum AVPixelFormat pix_fmts[] = { partitioner->format, AV_PIX_FMT_NONE };
	AVBufferSinkParams* buffersink_params = av_buffersink_params_alloc();
	buffersink_params->pixel_fmts = pix_fmts;
	err_code = avfilter_graph_create_filter(&partitioner->filter_buffersink_ctx, avfilter_get_by_name("buffersink"), 
		"out", NULL, buffersink_params, partitioner->filter_graph);
	if (err_code < 0) {
		blog(LOG_ERROR, "avfilter_graph_create_filter buffersink failed with error code %d", err_code);
		av_free(buffersink_params);
		goto fail;
	}
	av_free(buffersink_params);

	err_code = avfilter_link(partitioner->filter_buffer_ctx, 0, partitioner->filter_colorspacekey_ctx, 0);
	if (0 != err_code) {
		blog(LOG_ERROR, "avfilter_link source with colorspace key failed %d.", err_code);
		goto fail;
	}

	err_code = avfilter_link(partitioner->filter_colorspacekey_ctx, 0, partitioner->filter_buffersink_ctx, 0);
	if (0 != err_code) {
		blog(LOG_ERROR, "avfilter_link colorspace key with buffer sink failed %d.", err_code);
		goto fail;
	}

	err_code = avfilter_graph_config(partitioner->filter_graph, NULL);
	if (err_code < 0) {
		blog(LOG_ERROR, "avfilter_graph_config failed with error code %d", err_code);
		goto fail;
	}

	return partitioner;

fail:
	destroy_context_partition(&partitioner);
	return NULL;
}

void destroy_context_partition(video_partitioner** context) {
	blog(LOG_INFO, "free context partition resource.");

	if (!context ||!*context) {
		return;
	}

	avfilter_free((*context)->filter_buffer_ctx);
	avfilter_free((*context)->filter_buffersink_ctx);
	avfilter_free((*context)->filter_colorspacekey_ctx);
	avfilter_graph_free(&((*context)->filter_graph));

	av_free((*context)->input_frame);
	av_free((*context)->output_frame);

	bfree((*context));
	(*context) = NULL;
}

static void fill_avframe_from_obsframe(video_partitioner* context, struct obs_source_frame *obs_frame) {
	for (int i = 0; i < AV_NUM_DATA_POINTERS; i++) {
		context->input_frame->data[i] = obs_frame->data[i];
		context->input_frame->linesize[i] = obs_frame->linesize[i];
	}

	context->input_frame->format = context->format;
	context->input_frame->width = obs_frame->width;
	context->input_frame->height = obs_frame->height;
}

static void fill_obsframe(struct obs_source_frame* obs_frame, AVFrame* av_frame)
{
	switch (obs_frame->format) {
	case VIDEO_FORMAT_I420:
		memcpy(obs_frame->data[0], av_frame->data[0], av_frame->linesize[0] * av_frame->height);
		memcpy(obs_frame->data[1], av_frame->data[1], av_frame->linesize[1] * av_frame->height / 2);
		memcpy(obs_frame->data[2], av_frame->data[2], av_frame->linesize[2] * av_frame->height / 2);
		break;

	case VIDEO_FORMAT_NV12:
		memcpy(obs_frame->data[0], av_frame->data[0], av_frame->linesize[0] * av_frame->height);
		memcpy(obs_frame->data[1], av_frame->data[1], av_frame->linesize[1] * av_frame->height / 2);
		break;

	case VIDEO_FORMAT_I444:
		memcpy(obs_frame->data[0], av_frame->data[0], av_frame->linesize[0] * av_frame->height);
		memcpy(obs_frame->data[1], av_frame->data[1], av_frame->linesize[1] * av_frame->height);
		memcpy(obs_frame->data[2], av_frame->data[2], av_frame->linesize[2] * av_frame->height);
		break;

	case VIDEO_FORMAT_YVYU:
	case VIDEO_FORMAT_YUY2:
	case VIDEO_FORMAT_UYVY:
	case VIDEO_FORMAT_NONE:
	case VIDEO_FORMAT_RGBA:
	case VIDEO_FORMAT_BGRA:
	case VIDEO_FORMAT_BGRX:
		memcpy(obs_frame->data[0], av_frame->data[0], av_frame->linesize[0] * av_frame->height);
		break;
	}
}

static void fill_obsframe_from_avframe(struct obs_source_frame* obs_frame, AVFrame* av_frame) {
	if (av_frame) {
		obs_frame->format = convert_pixel_format(av_frame->format);
		blog(LOG_DEBUG, "video frame after context_partition with format(%s)", get_video_format_name(obs_frame->format));
		if (memcmp(obs_frame->data[0], av_frame->data[0], obs_frame->linesize[0] * obs_frame->height) != 0) {
			fill_obsframe(obs_frame, av_frame);
		}
	}
}

static bool frame_format_suitable(struct obs_source_frame* obs_frame) {
	switch (obs_frame->format) {
	case VIDEO_FORMAT_I420:
	case VIDEO_FORMAT_NV12:
	case VIDEO_FORMAT_I444:
	case VIDEO_FORMAT_YVYU:
	case VIDEO_FORMAT_YUY2:
	case VIDEO_FORMAT_UYVY:
	case VIDEO_FORMAT_RGBA:
	case VIDEO_FORMAT_BGRA:
	case VIDEO_FORMAT_BGRX:
		return true;
	}
	return false;
}

void do_context_partition(video_partitioner** context, struct obs_source_frame* obs_frame) {

	blog(LOG_DEBUG, "do_context_partition begin.");

	if (!context || !obs_frame) {
		return;
	}

	if (!frame_format_suitable(obs_frame)) {
		blog(LOG_ERROR, "do_context_partition failed with video format(%s)", get_video_format_name(obs_frame->format));
		return;
	}

	blog(LOG_DEBUG, "do_context_partition with video frame format(%s)", get_video_format_name(obs_frame->format));

	if (!*context) {
		*context = create_context_partition(obs_frame);
	}

	if (did_video_frame_property_changed(*context, obs_frame)) {
		destroy_context_partition(context);
		*context = create_context_partition(obs_frame);
	}

	if (!*context) {
		blog(LOG_ERROR, "failed to create video context partition.");
		return;
	}

	video_partitioner* ctx = *context;

	fill_avframe_from_obsframe(ctx, obs_frame);

	int64_t now = os_gettime_ns();

	int err_code = av_buffersrc_add_frame(ctx->filter_buffer_ctx, ctx->input_frame);
	if (err_code < 0) {
		blog(LOG_ERROR, "av_buffersrc_add_frame failed with error %d", err_code);
		return;
	}

	err_code = av_buffersink_get_frame(ctx->filter_buffersink_ctx, ctx->output_frame);
	if (err_code < 0) {
		blog(LOG_ERROR, "av_buffersink_get_frame failed with error %d", err_code);
		return;
	}

	int64_t after = os_gettime_ns();
	int64_t duration = (after - now) / 1000000.;
	blog(LOG_DEBUG, "context partition time used %d ms", duration);

	fill_obsframe_from_avframe(obs_frame, ctx->output_frame);
	av_frame_unref(ctx->output_frame);
	blog(LOG_DEBUG, "do_context_partition end.");
}
