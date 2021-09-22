#ifndef FFPLAYER_H
#define FFPLAYER_H

#include <QObject>
#include "ffplayerDefine.h"
#include <QImage>
#include <thread>

#include "types.h"
#include "Cond.h"
#include "VideoPlayerEventHandle.h"

class ffPlayer : public QObject
{
	Q_OBJECT
public:
	ffPlayer();
    ~ffPlayer();

    ///初始化播放器（必需要调用一次）
    static bool initPlayer();

    /**
     * @brief setVideoPlayerCallBack 设置播放器回调函数
     * @param pointer
     */
    void setVideoPlayerCallBack(VideoPlayerCallBack *pointer){mVideoPlayerCallBack=pointer;}

    bool startPlay(const std::string &filePath);

    bool replay(); //重新播放

    bool play(); //播放（用于暂停后，重新开始播放）
    bool pause(); //暂停播放
    bool stop(bool isWait = false); //停止播放-参数表示是否等待所有的线程执行完毕再返回

    void seek(int64_t pos); //单位是微秒

    void setMute(bool isMute){mIsMute = isMute;}
    void setVolume(float value);
    float getVolume(){return mVolume;}

    int64_t getTotalTime(); //单位微秒
    double getCurrentTime(); //单位秒

protected:
    void readVideoFile(); //读取视频文件
    void decodeVideoThread();

    static void sdlAudioCallBackFunc(void *userdata, Uint8 *stream, int len);
    void sdlAudioCallBack(Uint8 *stream, int len);
    int decodeAudioFrame(bool isBlock = false);

private:
    std::string mFilePath; //视频文件路径
    VideoPlayerState mPlayerState; //播放状态
    ///音量相关变量
    bool  mIsMute;
    float mVolume; //音量 0~1 超过1 表示放大倍数
    /// 跳转相关的变量
    int             seek_req = 0; //跳转标志
    int64_t         seek_pos; //跳转的位置 -- 微秒
    int             seek_flag_audio;//跳转标志 -- 用于音频线程中
    int             seek_flag_video;//跳转标志 -- 用于视频线程中
    double          seek_time; //跳转的时间(秒)  值和seek_pos是一样的

    ///播放控制相关
    bool mIsNeedPause; //暂停后跳转先标记此变量
    bool mIsPause;  //暂停标志
    bool mIsQuit;   //停止
    bool mIsReadFinished; //文件读取完毕
    bool mIsReadThreadFinished;
    bool mIsVideoThreadFinished; //视频解码线程
    bool mIsAudioThreadFinished; //音频播放线程

    ///音视频同步相关
    uint64_t mVideoStartTime; //开始播放视频的时间
    uint64_t mPauseStartTime; //暂停开始的时间
    double audio_clock; ///音频时钟
    double video_clock; ///<pts of last decoded frame / predicted pts of next decoded frame
    AVStream *mVideoStream; //视频流
    AVStream *mAudioStream; //音频流

    ///视频相关
    AVFormatContext *pFormatCtx;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;

    ///音频相关
    AVCodecContext *aCodecCtx;
    AVCodec *aCodec;
    AVFrame *aFrame;

    ///以下变量用于音频重采样
    /// 由于ffmpeg解码出来后的pcm数据有可能是带平面的pcm，因此这里统一做重采样处理，
    /// 重采样成44100的16 bits 双声道数据(AV_SAMPLE_FMT_S16)
    AVFrame *aFrame_ReSample;
    SwrContext *swrCtx;

    enum AVSampleFormat in_sample_fmt; //输入的采样格式
    enum AVSampleFormat out_sample_fmt;//输出的采样格式 16bit PCM
    int in_sample_rate;//输入的采样率
    int out_sample_rate;//输出的采样率
    int audio_tgt_channels; ///av_get_channel_layout_nb_channels(out_ch_layout);
    int out_ch_layout;
    unsigned int audio_buf_size;
    unsigned int audio_buf_index;
    DECLARE_ALIGNED(16,uint8_t,audio_buf) [AVCODEC_MAX_AUDIO_FRAME_SIZE * 4];

    int autorotate = 1;
    int find_stream_info = 1;
    int filter_nbthreads = 0;

#if CONFIG_AVFILTER
    const char **vfilters_list = NULL;
    int nb_vfilters = 0;
    char *afilters = NULL;

    int vfilter_idx;
    AVFilterContext *in_video_filter;   // the first filter in the video chain
    AVFilterContext *out_video_filter;  // the last filter in the video chain
//    AVFilterContext *in_audio_filter;   // the first filter in the audio chain
//    AVFilterContext *out_audio_filter;  // the last filter in the audio chain
//    AVFilterGraph *agraph;              // audio filter graph
#endif

    ///视频帧队列
    Cond *mConditon_Video;
    std::list<AVPacket> mVideoPacktList;
    bool inputVideoQuene(const AVPacket &pkt);
    void clearVideoQuene();

    ///音频帧队列
    Cond *mConditon_Audio;
    std::list<AVPacket> mAudioPacktList;
    bool inputAudioQuene(const AVPacket &pkt);
    void clearAudioQuene();

    ///本播放器中SDL仅用于播放音频，不用做别的用途
    ///SDL播放音频相关
    SDL_AudioDeviceID mAudioID;
    int openSDL();
    void closeSDL();

    int configure_filtergraph(AVFilterGraph *graph, const char *filtergraph, AVFilterContext *source_ctx, AVFilterContext *sink_ctx);
    int configure_video_filters(AVFilterGraph *graph, const char *vfilters, AVFrame *frame);

signals:
	void sigTotalTimeChanged(int64_t uSec);

    ///回调函数相关，主要用于输出信息给界面
private:
    ///回调函数
    VideoPlayerCallBack *mVideoPlayerCallBack;

    ///打开文件失败
    void doOpenVideoFileFailed(const int &code = 0);

    ///打开sdl失败的时候回调此函数
    void doOpenSdlFailed(const int &code);

    ///获取到视频时长的时候调用此函数
    void doTotalTimeChanged(const int64_t &uSec);

    ///播放器状态改变的时候回调此函数
    void doPlayerStateChanged(const VideoPlayerState &state, const bool &hasVideo, const bool &hasAudio);

    ///显示视频数据，此函数不宜做耗时操作，否则会影响播放的流畅性。
    void doDisplayVideo(const uint8_t *yuv420Buffer, const int &width, const int &height);

};

#endif
