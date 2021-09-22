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
	QString hash; //mv��ϣ
	QString mvName; //mv����
	QString url; //����
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
	//�����ƶ�����ֵ
	QPoint m_point;
	volatile bool m_bMove = false;
	//����������
	QMediaPlayer* m_videoPlayer = nullptr;
	//ffmepg�����߳�
	//ffPlayer *m_ffPlayer = nullptr;
	//�б�
	QMediaPlaylist *m_videoPlayList = nullptr;
	//���Ŵ���
	QVideoWidget* m_videoPlayWidget = nullptr;
	//������
	QVBoxLayout* layout_video = nullptr;
	//MV����
	QNetworkAccessManager *m_netWorkMvDownLoad = nullptr;
	//�������ݶ������
	bool m_isWorking = false;
	//���ݻ���
	MvInfoPlayer m_mvInfoPlayer;
	//��Ƶ�ؼ�
	QSharedPointer<VideoControls> m_videoControls = nullptr;
	//��Ƶʱ���ַ���
	QString m_videoTime;
	//�رհ�ť
	QSharedPointer<VideoControlTop> m_videoControlTops = nullptr;

	VideoFramePtr m_videoFrame;
	QList<FaceInfoNode> mFaceInfoList;
	//OPenGL���ڻ���ͼ��
	GLuint textureUniformY; //y��������λ��
	GLuint textureUniformU; //u��������λ��
	GLuint textureUniformV; //v��������λ��
	GLuint id_y; //y�������ID
	GLuint id_u; //u�������ID
	GLuint id_v; //v�������ID
	QOpenGLTexture* m_pTextureY;  //y�������
	QOpenGLTexture* m_pTextureU;  //u�������
	QOpenGLTexture* m_pTextureV;  //v�������
	QOpenGLShader *m_pVSHader;  //������ɫ���������
	QOpenGLShader *m_pFSHader;  //Ƭ����ɫ������
	QOpenGLShaderProgram *m_pShaderProgram; //��ɫ����������
	GLfloat *m_vertexVertices; // �������

	float mPicIndex_X; //��������ʾ����� ͼ��ƫ�����ٷֱ� (����ڴ��ڴ�С��)
	float mPicIndex_Y; //
	int m_nVideoW; //��Ƶ�ֱ��ʿ�
	int m_nVideoH; //��Ƶ�ֱ��ʸ�

	//OpenGL���ڻ��ƾ���
	bool mIsShowFaceRect;
	GLuint m_posAttr;
	GLuint m_colAttr;
	QOpenGLShaderProgram *m_program;
	//openGL��ʼ�������Ƿ�ִ�й���
	bool mIsOpenGLInited;
	//��һ�λ�ȡ��֡��ʱ���
	qint64 mLastGetFrameTime;
	//��ǰģʽ�Ƿ��ǰ����� ����⵽��ȫ�ֱ�����һ�µ�ʱ�� ����������openGL����
	bool mCurrentVideoKeepAspectRatio;

protected:
	///��ʾ��Ƶ���ݣ��˺�����������ʱ�����������Ӱ�첥�ŵ������ԡ�
	void onDisplayVideo(VideoFramePtr videoFrame);
};
