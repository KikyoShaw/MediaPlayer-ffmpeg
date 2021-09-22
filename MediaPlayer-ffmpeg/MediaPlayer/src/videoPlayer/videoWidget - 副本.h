#pragma once

#include <QWidget>
#include "ui_videoWidget.h"
#include "VideoFrame.h"
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>

#include "ffPlayer.h"

class QMediaPlayer;
class QMediaPlaylist;
class QVideoWidget;
class QVBoxLayout;
class QNetworkAccessManager;
class QNetworkReply;
class VideoControls;
class VideoControlTop;

struct MvInfoPlayer
{
	QString hash; //mv哈希
	QString mvName; //mv名称
	QString url; //链接
	QString path;
};

struct FaceInfoNode
{
	QRect faceRect;
};

class VideoWidget : /*public QWidget*/public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit VideoWidget(QWidget *parent = 0);
    ~VideoWidget();

	void setVideoPlay(const QString &filePath);

	void setMvPlay(MvInfoPlayer info);

	void inputOneFrame(VideoFramePtr videoFrame);

private:
	void initVideoControls();
	void locateWidgets();

	void resetGLVertex(int window_W, int window_H);

private slots:
	void sltNetWorkMvDownLoad(QNetworkReply *reply);
	void sltVideoPlayOrPause(bool state);
	void sltSoundVoiceValue(int value);
	void sltSetPosition(int value);
	void sltDurationChanged(qint64 duration);
	void sltPositionChanged(qint64 position);

protected:
	virtual void mouseMoveEvent(QMouseEvent *event);
	virtual void mousePressEvent(QMouseEvent *event);
	virtual void mouseReleaseEvent(QMouseEvent *event);
	virtual void showEvent(QShowEvent *event);
	virtual void closeEvent(QCloseEvent *event);
	virtual void moveEvent(QMoveEvent *event);
	virtual void hideEvent(QHideEvent *event);
	virtual void resizeEvent(QResizeEvent *event);
	virtual bool eventFilter(QObject *obj, QEvent *event);

	void initializeGL() Q_DECL_OVERRIDE;
	void resizeGL(int window_W, int window_H) Q_DECL_OVERRIDE;
	void paintGL() Q_DECL_OVERRIDE;

private:
    Ui::mainVideo ui;
	//窗口移动属性值
	QPoint m_point;
	volatile bool m_bMove = false;
	//播放器对象
	QMediaPlayer* m_videoPlayer = nullptr;
	//ffmepg播放线程
	//ffPlayer *m_ffPlayer = nullptr;
	//列表
	QMediaPlaylist *m_videoPlayList = nullptr;
	//播放窗口
	QVideoWidget* m_videoPlayWidget = nullptr;
	//布局器
	QVBoxLayout* layout_video = nullptr;
	//MV下载
	QNetworkAccessManager *m_netWorkMvDownLoad = nullptr;
	//避免数据多次请求
	bool m_isWorking = false;
	//数据缓存
	MvInfoPlayer m_mvInfoPlayer;
	//视频控件
	QSharedPointer<VideoControls> m_videoControls = nullptr;
	//视频时间字符串
	QString m_videoTime;
	//关闭按钮
	QSharedPointer<VideoControlTop> m_videoControlTops = nullptr;

	VideoFramePtr m_videoFrame;
	QList<FaceInfoNode> mFaceInfoList;
	//OPenGL用于绘制图像
	GLuint textureUniformY; //y纹理数据位置
	GLuint textureUniformU; //u纹理数据位置
	GLuint textureUniformV; //v纹理数据位置
	GLuint id_y; //y纹理对象ID
	GLuint id_u; //u纹理对象ID
	GLuint id_v; //v纹理对象ID
	QOpenGLTexture* m_pTextureY;  //y纹理对象
	QOpenGLTexture* m_pTextureU;  //u纹理对象
	QOpenGLTexture* m_pTextureV;  //v纹理对象
	QOpenGLShader *m_pVSHader;  //顶点着色器程序对象
	QOpenGLShader *m_pFSHader;  //片段着色器对象
	QOpenGLShaderProgram *m_pShaderProgram; //着色器程序容器
	GLfloat *m_vertexVertices; // 顶点矩阵

	float mPicIndex_X; //按比例显示情况下 图像偏移量百分比 (相对于窗口大小的)
	float mPicIndex_Y; //
	int m_nVideoW; //视频分辨率宽
	int m_nVideoH; //视频分辨率高

	//OpenGL用于绘制矩形
	bool mIsShowFaceRect;
	GLuint m_posAttr;
	GLuint m_colAttr;
	QOpenGLShaderProgram *m_program;
	//openGL初始化函数是否执行过了
	bool mIsOpenGLInited;
	//上一次获取到帧的时间戳
	qint64 mLastGetFrameTime;
	//当前模式是否是按比例 当检测到与全局变量不一致的时候 则重新设置openGL矩阵
	bool mCurrentVideoKeepAspectRatio;

protected:
	///显示视频数据，此函数不宜做耗时操作，否则会影响播放的流畅性。
	void onDisplayVideo(VideoFramePtr videoFrame);
};
