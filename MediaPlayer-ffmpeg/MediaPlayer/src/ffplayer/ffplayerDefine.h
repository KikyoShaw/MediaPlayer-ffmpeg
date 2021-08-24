#pragma once

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/time.h>
#include <libavutil/pixfmt.h>
#include <libavutil/display.h>
#include <libavutil/avstring.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/imgutils.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>

#include <SDL.h>
#include <SDL_audio.h>
#include <SDL_types.h>
#include <SDL_name.h>
#include <SDL_main.h>
#include <SDL_config.h>
}

//启用滤镜，用于旋转带角度的视频
#define CONFIG_AVFILTER 1
#define SDL_AUDIO_BUFFER_SIZE 1024
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio
#define MAX_AUDIO_SIZE (50 * 20)
#define MAX_VIDEO_SIZE (25 * 20)
#define FLUSH_DATA "FLUSH"

enum E_PlayState
{
	E_Stop = 1, //未播放状态
	E_Play = 2, //播放状态
	E_Pause = 3, //暂停状态
};

enum FFplayerCodeEnum
{
	E_OpenFileFail = 2000, //打开文件失败
	E_OpenSDLFail = 2001,  //打开SDL失败
};
