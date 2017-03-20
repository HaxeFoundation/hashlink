#define HL_NAME(n) video_##n
#include <hl.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

typedef struct {
	void *finalizer;
	AVFormatContext *input;
	AVCodecContext *codec;
	AVFrame *frame;
	int videoStream;
	struct SwsContext *scale;
} hl_video;

static void hl_video_free( hl_video *v ) {
	avformat_close_input(&v->input);
	avcodec_free_context(&v->codec);
	av_frame_free(&v->frame);
	if( v->scale ) {
		sws_freeContext(v->scale);
		v->scale = NULL;
	}
}

HL_PRIM void HL_NAME(video_init)() {
	av_register_all();
}

static bool hl_video_init( hl_video *v, const char *file ) {
	AVCodec *codec;
	AVCodecContext *codecOrig;
	int i;
	if( avformat_open_input(&v->input, file, NULL, NULL) )
		return false;
	if( avformat_find_stream_info(v->input, NULL) < 0 )
		return false;
	v->videoStream = -1;
	for(i=0;i<(int)v->input->nb_streams; i++)
		if( v->input->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO ) {
		    v->videoStream = i;
			break;
		}
	if( v->videoStream < 0 )
		return false;
	codecOrig = v->input->streams[v->videoStream]->codec;
	codec = avcodec_find_decoder(codecOrig->codec_id);
	if( codec == NULL )
		return false;
	v->codec = avcodec_alloc_context3(codec);
	avcodec_copy_context(v->codec, codecOrig);
	if( avcodec_open2(v->codec, codec,NULL) < 0 )
		return false;
	v->frame = av_frame_alloc();
	v->scale = sws_getContext(v->codec->width,v->codec->height, v->codec->pix_fmt, v->codec->width, v->codec->height, AV_PIX_FMT_RGBA, SWS_BILINEAR, NULL, NULL, NULL);
	return true;
}

HL_PRIM hl_video *HL_NAME(video_open)( const char *file ) {
	hl_video *v;
	v = hl_gc_alloc_finalizer(sizeof(hl_video));
	memset(v,0,sizeof(hl_video));
	v->finalizer = hl_video_free;
	if( !hl_video_init(v,file) ) {
		hl_video_free(v);
		return NULL;
	}
	return v;
}

HL_PRIM void HL_NAME(video_get_size)( hl_video *v, int *width, int *height ) {
	*width = v->codec->width;
	*height = v->codec->height;
}

HL_PRIM bool HL_NAME(video_decode_frame)( hl_video *v, vbyte *out, double *time ) {
	AVPacket packet;
	AVPicture i;
	int frameFinished = 0;
	while( !frameFinished ) {
		if( av_read_frame(v->input, &packet) < 0 )
			return false;
		if( packet.stream_index == v->videoStream && avcodec_decode_video2(v->codec, v->frame, &frameFinished, &packet) < 0 ) {
			av_free_packet(&packet);
			return false;
		}
		av_free_packet(&packet);
	}
	// extract to output
	if( out ) {
		i.data[0] = out;
		i.data[1] = out + 1;
		i.data[2] = out + 2;
		i.linesize[0] = v->codec->width * 4;
		i.linesize[1] = v->codec->width * 4;
		i.linesize[2] = v->codec->width * 4;
		sws_scale(v->scale, v->frame->data, v->frame->linesize, 0, v->codec->height, i.data, i.linesize);
	}
	if( time )
		*time = (double)av_frame_get_best_effort_timestamp(v->frame) * av_q2d(v->codec->pkt_timebase);
	return true;
}

HL_PRIM void HL_NAME(video_close)( hl_video *v ) {
	hl_video_free(v);
}

#define _VIDEO _ABSTRACT(hl_video)

DEFINE_PRIM(_VOID, video_init, _NO_ARG);
DEFINE_PRIM(_VIDEO, video_open, _BYTES);
DEFINE_PRIM(_VOID, video_get_size, _VIDEO _REF(_I32) _REF(_I32));
DEFINE_PRIM(_BOOL, video_decode_frame, _VIDEO _BYTES _REF(_F64));
DEFINE_PRIM(_VOID, video_close, _VIDEO);