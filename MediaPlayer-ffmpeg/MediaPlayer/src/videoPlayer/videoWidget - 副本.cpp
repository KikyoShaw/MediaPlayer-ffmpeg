#include "videoWidget.h"
#include <QVideoWidget>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QMouseEvent>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QFileInfo>
#include <QDir>
#include "videoControls.h"
#include "videoControlTops.h"

#include "AppConfig.h"

#define ATTRIB_VERTEX 3
#define ATTRIB_TEXTURE 4

///���ڻ��ƾ���
//! [3]
static const char *vertexShaderSource =
	"attribute highp vec4 posAttr;\n"
	"attribute lowp vec4 colAttr;\n"
	"varying lowp vec4 col;\n"
	"uniform highp mat4 matrix;\n"
	"void main() {\n"
	"   col = colAttr;\n"
	"   gl_Position = posAttr;\n"
	"}\n";

static const char *fragmentShaderSource =
	"varying lowp vec4 col;\n"
	"void main() {\n"
	"   gl_FragColor = col;\n"
	"}\n";
	//! [3]

VideoWidget::VideoWidget(QWidget *parent) :
    /*QWidget(parent)*/
	QOpenGLWidget(parent)
{
    ui.setupUi(this);

    //���ر߿�
	setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowMinimizeButtonHint);

	ui.label_player->installEventFilter(this);

    //������Ƶ����
	m_videoPlayer = new QMediaPlayer(this);
	m_videoPlayList = new QMediaPlaylist(this);
	m_videoPlayList->setPlaybackMode(QMediaPlaylist::Sequential);
	m_videoPlayer->setVolume(50);
	m_videoPlayer->setPlaylist(m_videoPlayList);
    m_videoPlayWidget = new QVideoWidget();
	m_videoPlayWidget->setStyleSheet("border:none;background-color:transparent;");
	layout_video = new QVBoxLayout();
	layout_video->setMargin(0);
	ui.label_player->setLayout(layout_video);
	m_videoPlayWidget->resize(ui.label_player->size()); 
	m_videoPlayer->setVideoOutput(m_videoPlayWidget);
	layout_video->addWidget(m_videoPlayWidget);

	//m_ffPlayer = new ffPlayer();

	//����ͬ��
	connect(m_videoPlayer, SIGNAL(durationChanged(qint64)), this, SLOT(sltDurationChanged(qint64)));
	connect(m_videoPlayer, SIGNAL(positionChanged(qint64)), this, SLOT(sltPositionChanged(qint64)));

	initVideoControls();

	m_netWorkMvDownLoad = new QNetworkAccessManager(this);
	connect(m_netWorkMvDownLoad, &QNetworkAccessManager::finished, this, &VideoWidget::sltNetWorkMvDownLoad, Qt::DirectConnection);
}

VideoWidget::~VideoWidget()
{
}

void VideoWidget::setVideoPlay(const QString & filePath)
{
	m_videoPlayList->addMedia(QUrl::fromLocalFile(filePath));
	m_videoPlayer->play();
	if (m_videoControls) {
		m_videoControls->setPlaying();
	}
}

void VideoWidget::setMvPlay(MvInfoPlayer info)
{
	if (m_isWorking) return;
	m_isWorking = true;

	//m_ffPlayer->stop();
	//m_ffPlayer->startPlay(info.url.toStdString());
	return;

	if (m_videoPlayer->state() != QMediaPlayer::StoppedState) {
		m_videoPlayer->stop();
	}
	m_mvInfoPlayer = info;
	setWindowTitle(info.mvName);
	if (m_videoControlTops) {
		m_videoControlTops->setTitle(info.mvName);
	}
	//��װ·��,�����������������
	m_mvInfoPlayer.path = "out/" + info.hash + ".mp4";
	QFileInfo fileinfo(m_mvInfoPlayer.path);
	if (fileinfo.exists()) {
		setVideoPlay(m_mvInfoPlayer.path);
		m_isWorking = false;
		return;
	}
	QNetworkRequest request;
	//������������
	request.setUrl(QUrl(info.url));
	request.setHeader(QNetworkRequest::UserAgentHeader, "RT-Thread ART");
	m_netWorkMvDownLoad->get(request);
}

void VideoWidget::inputOneFrame(VideoFramePtr videoFrame)
{
	int width = videoFrame.get()->width();
	int height = videoFrame.get()->height();

	if (m_nVideoW <= 0 || m_nVideoH <= 0 || m_nVideoW != width || m_nVideoH != height)
	{
		if (width <= 0 || height <= 0) return;
		m_nVideoW = width;
		m_nVideoH = height;
		qDebug() << __FUNCTION__ << width << height << this->isHidden();
		if (mIsOpenGLInited){
			resetGLVertex(this->width(), this->height());
		}
	}

	mLastGetFrameTime = QDateTime::currentMSecsSinceEpoch();

	m_videoFrame.reset();
	m_videoFrame = videoFrame;

	update(); //����update��ִ�� paintEvent����
}

void VideoWidget::initVideoControls()
{
	m_videoControls = QSharedPointer<VideoControls>(new VideoControls(this));
	if (m_videoControls) {
		m_videoControls->setMvWidget();
		m_videoControls->installEventFilter(this);
		connect(m_videoControls.data(), &VideoControls::sigVideoPlayOrPause, this, &VideoWidget::sltVideoPlayOrPause);
		connect(m_videoControls.data(), &VideoControls::sigSetPosition, this, &VideoWidget::sltSetPosition);
		connect(m_videoControls.data(), &VideoControls::sigSoundVoiceValue, this, &VideoWidget::sltSoundVoiceValue);
	}

	m_videoControlTops = QSharedPointer<VideoControlTop>(new VideoControlTop(this));
	if (m_videoControlTops) {
		m_videoControlTops->installEventFilter(this);
		connect(m_videoControlTops.data(), &VideoControlTop::sigClose, this, [=]() {
			if (m_videoPlayer->state() != QMediaPlayer::StoppedState) {
				m_videoPlayer->stop();
			}
			close();
		});
	}
}

void VideoWidget::locateWidgets()
{
	if (m_videoControls) {
		int posX = ui.label_player->x() + 1;
		int posY = ui.label_player->height() - m_videoControls->height() - 1;
		m_videoControls->setFixedWidth(width() - posX - 2);
		m_videoControls->move(mapToGlobal(QPoint(posX, posY)));
		m_videoControls->locateWidgets();
	}

	if (m_videoControlTops) {
		int posX = ui.label_player->x() + 1;
		int posY = ui.label_player->y() + 1;
		m_videoControlTops->setFixedWidth(width() - posX - 2);
		m_videoControlTops->move(mapToGlobal(QPoint(posX, posY)));
	}
}

void VideoWidget::resetGLVertex(int window_W, int window_H)
{
	if (m_nVideoW <= 0 || m_nVideoH <= 0 || !AppConfig::gVideoKeepAspectRatio) //����
	{
		mPicIndex_X = 0.0;
		mPicIndex_Y = 0.0;

		// �������
		const GLfloat vertexVertices[] = {
			-1.0f, -1.0f,
			 1.0f, -1.0f,
			 -1.0f, 1.0f,
			 1.0f, 1.0f,
		};

		memcpy(m_vertexVertices, vertexVertices, sizeof(vertexVertices));

		//�������
		static const GLfloat textureVertices[] = {
			0.0f,  1.0f,
			1.0f,  1.0f,
			0.0f,  0.0f,
			1.0f,  0.0f,
		};
		//��������ATTRIB_VERTEX�Ķ������ֵ�Լ���ʽ
		glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, m_vertexVertices);
		//��������ATTRIB_TEXTURE���������ֵ�Լ���ʽ
		glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, 0, 0, textureVertices);
		//����ATTRIB_VERTEX���Ե�����,Ĭ���ǹرյ�
		glEnableVertexAttribArray(ATTRIB_VERTEX);
		//����ATTRIB_TEXTURE���Ե�����,Ĭ���ǹرյ�
		glEnableVertexAttribArray(ATTRIB_TEXTURE);
	}
	else //������
	{
		int pix_W = window_W;
		int pix_H = m_nVideoH * pix_W / m_nVideoW;

		int x = this->width() - pix_W;
		int y = this->height() - pix_H;

		x /= 2;
		y /= 2;

		if (y < 0)
		{
			pix_H = window_H;
			pix_W = m_nVideoW * pix_H / m_nVideoH;

			x = this->width() - pix_W;
			y = this->height() - pix_H;

			x /= 2;
			y /= 2;

		}

		mPicIndex_X = x * 1.0 / window_W;
		mPicIndex_Y = y * 1.0 / window_H;

		//qDebug()<<window_W<<window_H<<pix_W<<pix_H<<x<<y;
		float index_y = y * 1.0 / window_H * 2.0 - 1.0;
		float index_y_1 = index_y * -1.0;
		float index_y_2 = index_y;

		float index_x = x * 1.0 / window_W * 2.0 - 1.0;
		float index_x_1 = index_x * -1.0;
		float index_x_2 = index_x;

		const GLfloat vertexVertices[] = {
			index_x_2, index_y_2,
			index_x_1,  index_y_2,
			index_x_2, index_y_1,
			index_x_1,  index_y_1,
		};

		memcpy(m_vertexVertices, vertexVertices, sizeof(vertexVertices));

#if TEXTURE_HALF
		static const GLfloat textureVertices[] = {
			0.0f,  1.0f,
			0.5f,  1.0f,
			0.0f,  0.0f,
			0.5f,  0.0f,
		};
#else
		static const GLfloat textureVertices[] = {
			0.0f,  1.0f,
			1.0f,  1.0f,
			0.0f,  0.0f,
			1.0f,  0.0f,
		};
#endif
		//��������ATTRIB_VERTEX�Ķ������ֵ�Լ���ʽ
		glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, m_vertexVertices);
		//��������ATTRIB_TEXTURE���������ֵ�Լ���ʽ
		glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, 0, 0, textureVertices);
		//����ATTRIB_VERTEX���Ե�����,Ĭ���ǹرյ�
		glEnableVertexAttribArray(ATTRIB_VERTEX);
		//����ATTRIB_TEXTURE���Ե�����,Ĭ���ǹرյ�
		glEnableVertexAttribArray(ATTRIB_TEXTURE);
	}
}

void VideoWidget::sltVideoPlayOrPause(bool state)
{
	if (state) {
		m_videoPlayer->play();
	}
	else {
		m_videoPlayer->pause();
	}
}

void VideoWidget::sltSoundVoiceValue(int value)
{
	m_videoPlayer->setVolume(value);
}

void VideoWidget::sltSetPosition(int value)
{
	m_videoPlayer->setPosition(value);
}

void VideoWidget::sltDurationChanged(qint64 duration)
{
	//���ý�����
	if (m_videoControls) {
		m_videoControls->setProgressDuration(duration);
	}
	//��ʱ��
	auto hh = duration / 3600000;
	auto mm = (duration % 3600000) / 60000.0;
	auto ss = (duration % 60000) / 1000.0;
	QTime allTime(hh, mm, ss);
	m_videoTime = allTime.toString(tr("hh:mm:ss"));
}

void VideoWidget::sltPositionChanged(qint64 position)
{
	if (!m_videoControls) {
		return;
	}
	if (m_videoControls->isSliderDown()) {
		return;
	}
	m_videoControls->setSliderPosition(position);
	auto hh = position / 3600000;
	auto mm = (position % 3600000) / 60000.0;
	auto ss = (position % 60000) / 1000.0;
	QTime duration(hh, mm, ss);
	auto localTime = duration.toString(tr("hh:mm:ss"));
	auto text = QString("%1/%2").arg(localTime).arg(m_videoTime);
	m_videoControls->setProgressText(text);
}

void VideoWidget::sltNetWorkMvDownLoad(QNetworkReply * reply)
{
	//��ԭ����
	m_isWorking = false;
	//��ȡ��Ӧ����Ϣ��״̬��Ϊ200��ʾ����
	QVariant status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
	//�޴��󷵻�
	if (reply->error() == QNetworkReply::NoError) {
		QByteArray bytes = reply->readAll();  //��ȡ�ֽ�
		QFileInfo fileinfo(m_mvInfoPlayer.path);
		if (!fileinfo.exists()) {
			QDir().mkpath(fileinfo.absolutePath());
		}
		QFile file(m_mvInfoPlayer.path);
		if (!file.open(QFile::WriteOnly)) {
			return;
		}
		file.write(bytes);
		file.close();
		setVideoPlay(m_mvInfoPlayer.path);
	}
}

void VideoWidget::mouseMoveEvent(QMouseEvent * event)
{
	//�ж�����Ƿ񱻰��£�ֻ����������ˣ��䷵��ֵ����1(true)
	if ((event->buttons() & Qt::LeftButton) && m_bMove)
	{
		move(event->globalPos() - m_point);
	}
	QWidget::mouseMoveEvent(event);
}

void VideoWidget::mousePressEvent(QMouseEvent * event)
{
	//����¼��а�������ס���������
	if (event->button() == Qt::LeftButton)
	{
		m_bMove = true;
		//��ȡ�ƶ���λ����
		m_point = event->globalPos() - frameGeometry().topLeft();
	}
}

void VideoWidget::mouseReleaseEvent(QMouseEvent * event)
{
	m_bMove = false;
}

void VideoWidget::showEvent(QShowEvent * event)
{
	QWidget::showEvent(event);
	locateWidgets();
}

void VideoWidget::closeEvent(QCloseEvent * event)
{
	if (m_videoControls) {
		m_videoControls->closeWidget();
	}
	if (m_videoControlTops) {
		m_videoControlTops->close();
	}
	QWidget::closeEvent(event);
}

void VideoWidget::moveEvent(QMoveEvent * event)
{
	QWidget::moveEvent(event);
	locateWidgets();
}

void VideoWidget::hideEvent(QHideEvent * event)
{
	if (m_videoControls) {
		m_videoControls->setVisible(false);
	}
	if (m_videoControlTops) {
		m_videoControlTops->setVisible(false);
	}
	locateWidgets();
	QWidget::hideEvent(event);
}

void VideoWidget::resizeEvent(QResizeEvent * event)
{
	QWidget::resizeEvent(event);
	locateWidgets();
}

bool VideoWidget::eventFilter(QObject * obj, QEvent * event)
{
	if (obj == Q_NULLPTR) {
		return false;
	}
	if (m_videoControls == Q_NULLPTR) {
		return false;
	}
	if (m_videoControlTops == Q_NULLPTR) {
		return false;
	}

	if (QEvent::Enter == event->type()) {
		if (obj == ui.label_player || obj == m_videoControls || obj == m_videoControlTops) {
			/*if (QMediaPlayer::StoppedState != m_videoPlayer->state()) {*/
				m_videoControls->setVisible(true);
				m_videoControlTops->setVisible(true);
			/*}*/
			return false;
		}
	}
	else if (QEvent::Leave == event->type()) {
		if (obj != ui.label_player || obj != m_videoControls || obj != m_videoControlTops) {
			auto volumIsVisible = m_videoControls->getVolumVisible();
			if (!volumIsVisible) {
				m_videoControls->setVisible(false);
				m_videoControlTops->setVisible(false);
			}
			return false;
		}
	}
	return QWidget::eventFilter(obj, event);
}

void VideoWidget::onDisplayVideo(VideoFramePtr videoFrame)
{
	int width = videoFrame.get()->width();
	int height = videoFrame.get()->height();

	if (m_nVideoW <= 0 || m_nVideoH <= 0 || m_nVideoW != width || m_nVideoH != height)
	{
		if (width <= 0 || height <= 0) return;
		m_nVideoW = width;
		m_nVideoH = height;
		qDebug() << __FUNCTION__ << width << height << this->isHidden();
		if (mIsOpenGLInited) {
			resetGLVertex(this->width(), this->height());
		}
	}

	mLastGetFrameTime = QDateTime::currentMSecsSinceEpoch();

	m_videoFrame.reset();
	m_videoFrame = videoFrame;

	update(); //����update��ִ�� paintEvent����
}

void VideoWidget::initializeGL()
{
	qDebug() << __FUNCTION__ << m_videoFrame.get();

	mIsOpenGLInited = true;

	initializeOpenGLFunctions();
	glEnable(GL_DEPTH_TEST);
	//�ִ�opengl��Ⱦ����������ɫ���������������
	//��ɫ��������ʹ��openGL��ɫ����(OpenGL Shading Language, GLSL)��д��һ��С����,
	//       GLSL�ǹ�������OpenGL��ɫ��������,�����GLSL���Ե��﷨��Ҫ���߲����������
	//��ʼ��������ɫ�� ����
	m_pVSHader = new QOpenGLShader(QOpenGLShader::Vertex, this);
	//������ɫ��Դ��
	const char *vsrc = "attribute vec4 vertexIn; \
    attribute vec2 textureIn; \
    varying vec2 textureOut;  \
    void main(void)           \
    {                         \
        gl_Position = vertexIn; \
        textureOut = textureIn; \
    }";
	//���붥����ɫ������
	bool bCompile = m_pVSHader->compileSourceCode(vsrc);
	if (!bCompile)
	{
	}
	//��ʼ��Ƭ����ɫ�� ����gpu��yuvת����rgb
	m_pFSHader = new QOpenGLShader(QOpenGLShader::Fragment, this);
	//Ƭ����ɫ��Դ��(windows��opengl es ��Ҫ����float��仰)
	const char *fsrc =
#if defined(WIN32)
		"#ifdef GL_ES\n"
		"precision mediump float;\n"
		"#endif\n"
#else
#endif
		"varying vec2 textureOut; \
    uniform sampler2D tex_y; \
    uniform sampler2D tex_u; \
    uniform sampler2D tex_v; \
    void main(void) \
    { \
        vec3 yuv; \
        vec3 rgb; \
        yuv.x = texture2D(tex_y, textureOut).r; \
        yuv.y = texture2D(tex_u, textureOut).r - 0.5; \
        yuv.z = texture2D(tex_v, textureOut).r - 0.5; \
        rgb = mat3( 1,       1,         1, \
                    0,       -0.39465,  2.03211, \
                    1.13983, -0.58060,  0) * yuv; \
        gl_FragColor = vec4(rgb, 1); \
    }";
	//��glslԴ�����������������ɫ������
	bCompile = m_pFSHader->compileSourceCode(fsrc);
	if (!bCompile)
	{
	}

	//���ڻ��ƾ���
	m_program = new QOpenGLShaderProgram(this);
	m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
	m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
	m_program->link();
	m_posAttr = m_program->attributeLocation("posAttr");
	m_colAttr = m_program->attributeLocation("colAttr");


#define PROGRAM_VERTEX_ATTRIBUTE 0
#define PROGRAM_TEXCOORD_ATTRIBUTE 1
	//������ɫ����������
	m_pShaderProgram = new QOpenGLShaderProgram;
	//��Ƭ����ɫ����ӵ���������
	m_pShaderProgram->addShader(m_pFSHader);
	//��������ɫ����ӵ���������
	m_pShaderProgram->addShader(m_pVSHader);
	//������vertexIn��ָ��λ��ATTRIB_VERTEX,�������ڶ�����ɫԴ������������
	m_pShaderProgram->bindAttributeLocation("vertexIn", ATTRIB_VERTEX);
	//������textureIn��ָ��λ��ATTRIB_TEXTURE,�������ڶ�����ɫԴ������������
	m_pShaderProgram->bindAttributeLocation("textureIn", ATTRIB_TEXTURE);
	//���������������뵽����ɫ������
	m_pShaderProgram->link();
	//������������
	m_pShaderProgram->bind();
	//��ȡ��ɫ���е����ݱ���tex_y, tex_u, tex_v��λ��,��Щ����������������
	//Ƭ����ɫ��Դ���п��Կ���
	textureUniformY = m_pShaderProgram->uniformLocation("tex_y");
	textureUniformU = m_pShaderProgram->uniformLocation("tex_u");
	textureUniformV = m_pShaderProgram->uniformLocation("tex_v");

	// �������
	const GLfloat vertexVertices[] = {
		-1.0f, -1.0f,
		 1.0f, -1.0f,
		 -1.0f, 1.0f,
		 1.0f, 1.0f,
	};

	memcpy(m_vertexVertices, vertexVertices, sizeof(vertexVertices));

	//�������
	static const GLfloat textureVertices[] = {
		0.0f,  1.0f,
		1.0f,  1.0f,
		0.0f,  0.0f,
		1.0f,  0.0f,
	};

	///���ö�ȡ��YUV����Ϊ1�ֽڶ��䣬Ĭ��4�ֽڶ��룬
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	//��������ATTRIB_VERTEX�Ķ������ֵ�Լ���ʽ
	glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, m_vertexVertices);
	//��������ATTRIB_TEXTURE���������ֵ�Լ���ʽ
	glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, 0, 0, textureVertices);
	//����ATTRIB_VERTEX���Ե�����,Ĭ���ǹرյ�
	glEnableVertexAttribArray(ATTRIB_VERTEX);
	//����ATTRIB_TEXTURE���Ե�����,Ĭ���ǹرյ�
	glEnableVertexAttribArray(ATTRIB_TEXTURE);
	//�ֱ𴴽�y,u,v�������
	m_pTextureY = new QOpenGLTexture(QOpenGLTexture::Target2D);
	m_pTextureU = new QOpenGLTexture(QOpenGLTexture::Target2D);
	m_pTextureV = new QOpenGLTexture(QOpenGLTexture::Target2D);
	m_pTextureY->create();
	m_pTextureU->create();
	m_pTextureV->create();
	//��ȡ����y��������������ֵ
	id_y = m_pTextureY->textureId();
	//��ȡ����u��������������ֵ
	id_u = m_pTextureU->textureId();
	//��ȡ����v��������������ֵ
	id_v = m_pTextureV->textureId();
	glClearColor(0.0, 0.0, 0.0, 0.0);//���ñ���ɫ-��ɫ
	//qDebug("addr=%x id_y = %d id_u=%d id_v=%d\n", this, id_y, id_u, id_v);
}

void VideoWidget::resizeGL(int window_W, int window_H)
{
	mLastGetFrameTime = QDateTime::currentMSecsSinceEpoch();

	//    qDebug()<<__FUNCTION__<<m_pBufYuv420p<<window_W<<window_H;
	if (window_H == 0)// ��ֹ�����
	{
		window_H = 1;// ������Ϊ1
	}
	//�����ӿ�
	glViewport(0, 0, window_W, window_H);

	resetGLVertex(window_W, window_H);
}

void VideoWidget::paintGL()
{
	//qDebug()<<__FUNCTION__<<mCameraName<<m_pBufYuv420p;

	mLastGetFrameTime = QDateTime::currentMSecsSinceEpoch();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	///�����а����������ı� ����Ҫ����x yƫ��
	if (mCurrentVideoKeepAspectRatio != AppConfig::gVideoKeepAspectRatio)
	{
		mCurrentVideoKeepAspectRatio = AppConfig::gVideoKeepAspectRatio;
		resetGLVertex(this->width(), this->height());
	}

	///���ƾ��ο�
	if (mIsShowFaceRect && !mFaceInfoList.isEmpty())
	{
		m_program->bind();

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);

		for (int i = 0; i < mFaceInfoList.size(); i++)
		{
			FaceInfoNode faceNode = mFaceInfoList.at(i);
			QRect rect = faceNode.faceRect;

			int window_W = this->width();
			int window_H = this->height();

			int pix_W = rect.width();
			int pix_H = rect.height();

			int x = rect.x();
			int y = rect.y();

			float index_x_1 = x * 1.0 / m_nVideoW * 2.0 - 1.0;
			float index_y_1 = 1.0 - (y *1.0 / m_nVideoH * 2.0);

			float index_x_2 = (x + pix_W) * 1.0 / m_nVideoW * 2.0 - 1.0;
			float index_y_2 = index_y_1;

			float index_x_3 = index_x_2;
			float index_y_3 = 1.0 - ((y + pix_H) * 1.0 / m_nVideoH * 2.0);

			float index_x_4 = index_x_1;
			float index_y_4 = index_y_3;

			index_x_1 += mPicIndex_X;
			index_x_2 += mPicIndex_X;
			index_x_3 += mPicIndex_X;
			index_x_4 += mPicIndex_X;

			index_y_1 -= mPicIndex_Y;
			index_y_2 -= mPicIndex_Y;
			index_y_3 -= mPicIndex_Y;
			index_y_4 -= mPicIndex_Y;

			const GLfloat vertices[] = {
				index_x_1, index_y_1,
				index_x_2,  index_y_2,
				index_x_3, index_y_3,
				index_x_4,  index_y_4,
			};

			//            #7AC451 - 122, 196, 81 - 0.47843f, 0.768627f, 0.317647f
			const GLfloat colors[] = {
				0.47843f, 0.768627f, 0.317647f,
				0.47843f, 0.768627f, 0.317647f,
				0.47843f, 0.768627f, 0.317647f,
				0.47843f, 0.768627f, 0.317647f
			};

			glVertexAttribPointer(m_posAttr, 2, GL_FLOAT, GL_FALSE, 0, vertices);
			glVertexAttribPointer(m_colAttr, 3, GL_FLOAT, GL_FALSE, 0, colors);

			glLineWidth(2.2f); //���û��ʿ��

			glDrawArrays(GL_LINE_LOOP, 0, 4);
		}

		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(0);

		m_program->release();
	}

	VideoFrame * videoFrame = m_videoFrame.get();

	if (videoFrame != nullptr)
	{
		uint8_t *m_pBufYuv420p = videoFrame->buffer();

		if (m_pBufYuv420p != NULL)
		{
			m_pShaderProgram->bind();

			//����y��������
			//��������ԪGL_TEXTURE0
			glActiveTexture(GL_TEXTURE0);
			//ʹ������y������������
			glBindTexture(GL_TEXTURE_2D, id_y);
			//ʹ���ڴ���m_pBufYuv420p���ݴ���������y��������
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_nVideoW, m_nVideoH, 0, GL_RED, GL_UNSIGNED_BYTE, m_pBufYuv420p);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			//����u��������
			glActiveTexture(GL_TEXTURE1);//��������ԪGL_TEXTURE1
			glBindTexture(GL_TEXTURE_2D, id_u);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_nVideoW / 2, m_nVideoH / 2, 0, GL_RED, GL_UNSIGNED_BYTE, (char*)m_pBufYuv420p + m_nVideoW * m_nVideoH);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			//����v��������
			glActiveTexture(GL_TEXTURE2);//��������ԪGL_TEXTURE2
			glBindTexture(GL_TEXTURE_2D, id_v);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_nVideoW / 2, m_nVideoH / 2, 0, GL_RED, GL_UNSIGNED_BYTE, (char*)m_pBufYuv420p + m_nVideoW * m_nVideoH * 5 / 4);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			//ָ��y����Ҫʹ����ֵ ֻ����0,1,2�ȱ�ʾ����Ԫ������������opengl�����Ի��ĵط�
			//0��Ӧ����ԪGL_TEXTURE0 1��Ӧ����ԪGL_TEXTURE1 2��Ӧ����ĵ�Ԫ
			glUniform1i(textureUniformY, 0);
			//ָ��u����Ҫʹ����ֵ
			glUniform1i(textureUniformU, 1);
			//ָ��v����Ҫʹ����ֵ
			glUniform1i(textureUniformV, 2);
			//ʹ�ö������鷽ʽ����ͼ��
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			m_pShaderProgram->release();
		}
	}
}
