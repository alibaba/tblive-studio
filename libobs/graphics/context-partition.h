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

#pragma once

#if defined(WIN32) && !defined(__cplusplus)  
#define inline __inline  
#endif 

#include <libavfilter/avfilter.h>
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>

#include "obs.h"


struct video_info {
	uint32_t            width;
	uint32_t            height;
	enum video_format   format;
};

enum partition_type {
	chroma_key = 0,
	color_key
};

typedef struct partition_para_t {
	enum partition_type type;
	char color[128];
	double similarity;
	double blend;
}partition_param;

typedef struct ffmpeg_video_context_partition_t {
	AVFilterGraph* filter_graph;
	AVFilterContext* filter_buffer_ctx;
	AVFilterContext* filter_buffersink_ctx;
	AVFilterContext* filter_colorspacekey_ctx;
	enum AVPixelFormat format;
	AVFrame* input_frame;
	AVFrame* output_frame;
	char input_filter_config[256];
	char colorspacekey_filter_config[256];
	struct video_info info;
	partition_param param;
}video_partitioner;

void destroy_context_partition(video_partitioner** context);
void do_context_partition(video_partitioner** context, struct obs_source_frame* frame);

