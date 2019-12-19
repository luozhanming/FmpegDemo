//
// Created by Administrator on 2019/12/16.
//

#include <libavutil/imgutils.h>
#include <pthread.h>
#include "cn_luozhanming_opengldemo_JniTest.h""
#include "libavcodec/avcodec.h"
#include "mlog.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"


typedef struct {
    const char *input;
    const char *output;
    AVFormatContext *pFormatCtx;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    int video_stream_num;
    int *video_stream_index;
    int audio_stream_num;
    int *audio_stream_index;
    int isDecoding;
    pthread_t decode_send_thread;
    pthread_t decode_receive_thread;
} FFmpegGlobal;


FFmpegGlobal *mGlobal;


//Output FFmpeg's av_log()
void custom_log(void *ptr, int level, const char *fmt, va_list vl) {
    FILE *fp = fopen("/storage/emulated/0/av_log.txt", "a+");
    if (fp) {
        vfprintf(fp, fmt, vl);
        fflush(fp);
        fclose(fp);
    }
}

static void pgm_save(unsigned char *buf, int wrap, int xsize, int ysize,
                     const char *filename)
{
    FILE *f;
    int i;
    f = fopen(filename,"w");
    fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);
    for (i = 0; i < ysize; i++)
        fwrite(buf + i * wrap, 1, xsize, f);
    fclose(f);
}

/**
 * 送解线程
 * */
void send_decode_data(void *ptr) {
    int ret = -1;
    FILE *output_file = fopen(mGlobal->output,"wb+");
    char buf[1024] = {};
    AVPacket *packet;
    packet = av_packet_alloc();
    if (!packet) {
        LOGE("Could not alloc packet.");
        return;
    }
    AVFrame *frame;
    frame = av_frame_alloc();
    if (!frame) {
        LOGE("Could not alloc frame.");
        return;
    }
    int frame_count = 0;
    int isEnd = 0;
    while (mGlobal->isDecoding && isEnd == 0) {
        ret = av_read_frame(mGlobal->pFormatCtx, packet);
        if (ret >= 0) {
            //视频
            if (packet->stream_index == *mGlobal->video_stream_index) {
                LOGE("Read packet %d,Packet data size %d,Packet stream index %d", frame_count,
                     packet->size, packet->stream_index);
                frame_count++;
                if (avcodec_send_packet(mGlobal->pCodecCtx, packet) == 0) {
                    LOGE("视频送解码成功");
                }
            }
        } else {
            LOGE("读到结尾了");
            isEnd = 1;
        }

        ret = avcodec_receive_frame(mGlobal->pCodecCtx, frame);
        if (ret == 0) {
            LOGE("Frame data size %d,Frame format %d,Frame width %d,isKeyFrame:%d,AVPictureType %d,linesize %d,pkg_pts:%llu,pts %llu,dts:%llu",
                 frame->pkt_size, frame->format, frame->width, frame->key_frame, frame->pict_type,
                 frame->linesize[0],frame->pkt_pts,frame->pts,frame->pkt_dts);
            size_t y_size = mGlobal->pCodecCtx->width*mGlobal->pCodecCtx->height;
            fwrite(frame->data[0],1,y_size,output_file);    //Y
            fwrite(frame->data[1],1,y_size/4,output_file);  //U
            fwrite(frame->data[2],1,y_size/4,output_file);  //V

        }
    }
    //flush decode
    ret = avcodec_send_packet(mGlobal->pCodecCtx, NULL);
    while (ret >= 0) {
        ret = avcodec_receive_frame(mGlobal->pCodecCtx, frame);
        if (ret == 0) {
            LOGE("Frame data size %d,Frame format %d,Frame width %d,isKeyFrame:%d,AVPictureType %d,linesize %d,pkg_pts:%llu,pts %llu,dts:%llu",
                 frame->pkt_size, frame->format, frame->width, frame->key_frame, frame->pict_type,
                 frame->linesize[0],frame->pkt_pts,frame->pts,frame->pkt_dts);
            size_t y_size = mGlobal->pCodecCtx->width*mGlobal->pCodecCtx->height;
            fwrite(frame->data[0],1,y_size,output_file);    //Y
            fwrite(frame->data[1],1,y_size/4,output_file);  //U
            fwrite(frame->data[2],1,y_size/4,output_file);  //V
        }
    }
    fclose(output_file);
    av_packet_free(packet);
    av_frame_free(frame);
}



JNIEXPORT jstring JNICALL Java_cn_luozhanming_opengldemo_FFmpgeTest_jniString(
        JNIEnv *env,
        jobject obj) {
    char *info;
    info = (char *) malloc(10000 * sizeof(char));
    if (!info) {
        return (*env)->NewStringUTF(env, "No space at all.");
    }
    sprintf(info, "%s\n", avcodec_configuration());
    return (*env)->NewStringUTF(env, info);
}


/**
 * 将输入媒体流转化为其他格式
 * */
jint JNICALL Java_cn_luozhanming_opengldemo_FFmpgeTest_decode(
        JNIEnv *env,
        jobject obj,
        jstring input_jstr,
        jstring output_jstr) {
    /*1.解协议*/
    mGlobal = (FFmpegGlobal *) malloc(sizeof(FFmpegGlobal));
    mGlobal->input = (*env)->GetStringUTFChars(env, input_jstr, NULL);
    mGlobal->output = (*env)->GetStringUTFChars(env, output_jstr, NULL);
    //初始化ffmpeg
    av_log_set_callback(custom_log);
    av_register_all();
    avformat_network_init();
//    //分配空间
    mGlobal->pFormatCtx = avformat_alloc_context();
    if (!mGlobal->pFormatCtx) {
        LOGE("Could not alloc AVFormatContext.");
        return -1;
    }
    //打开输入流
    if (avformat_open_input(&(mGlobal->pFormatCtx),  mGlobal->input, NULL, NULL) != 0) {
        LOGE("Could not open %s",  mGlobal->input);
        return -1;
    }
    if (avformat_find_stream_info(mGlobal->pFormatCtx, NULL) < 0) {
        LOGE("Couldn't find stream information.");
        return -1;
    }

    /*2.视频解码*/
    //分配音视频Stream类型
    mGlobal->audio_stream_index = (int *) malloc(sizeof(int) * 10);
    mGlobal->video_stream_index = (int *) malloc(sizeof(int) * 10);
    mGlobal->video_stream_num = 0;
    mGlobal->audio_stream_num = 0;
    for (int i = 0; i < mGlobal->pFormatCtx->nb_streams; ++i) {
        if (mGlobal->pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            *(mGlobal->video_stream_index + mGlobal->video_stream_num * sizeof(int)) = i;
            LOGE("Video stream index %d",
                 *(mGlobal->video_stream_index + mGlobal->video_stream_num * sizeof(int)));
            mGlobal->video_stream_num++;
            LOGE("Video stream num %d", mGlobal->video_stream_num);
        } else if (mGlobal->pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            *(mGlobal->audio_stream_index + mGlobal->audio_stream_num * sizeof(int)) = i;
            LOGE("Audio stream index %d",
                 *(mGlobal->audio_stream_index + mGlobal->audio_stream_num * sizeof(int)));
            mGlobal->audio_stream_num++;
            LOGE("Audio stream num %d", mGlobal->audio_stream_num);
        }
    }

    AVDictionary *opts = NULL;
    mGlobal->pCodec = avcodec_find_decoder(
            mGlobal->pFormatCtx->streams[*mGlobal->video_stream_index]->codecpar->codec_id);
    if (!mGlobal->pCodec) {
        LOGE("Couldn't find video decoder.");
        return -1;
    }
    av_dump_format(mGlobal->pFormatCtx, 0,  mGlobal->input, 0);
    //为解码器分配上下文空间
    mGlobal->pCodecCtx = avcodec_alloc_context3(mGlobal->pCodec);
//    // 将AVStream里面的参数复制到上下文当中
    avcodec_parameters_to_context(mGlobal->pCodecCtx,
                                  mGlobal->pFormatCtx->streams[*mGlobal->video_stream_index]->codecpar);
    mGlobal->pCodecCtx->thread_count = 8;

    if (avcodec_open2(mGlobal->pCodecCtx, mGlobal->pCodec, &opts) != 0) {
        LOGE("Couldn't open video decoder.");
        return -1;
    }
    mGlobal->isDecoding = 1;
    if (pthread_create(&mGlobal->decode_send_thread, NULL, (void *) &send_decode_data,
                       (void *) "send") ==
        0) {
        LOGE("送解线程创建成功");
    }

    // decode();
    return 0;

}




