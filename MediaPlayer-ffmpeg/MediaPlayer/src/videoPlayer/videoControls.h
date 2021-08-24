#pragma once

#include <QWidget>
#include "ui_videoControls.h"

class VSliderWidget;

class VideoControls : public QWidget
{
	Q_OBJECT
public:
	VideoControls(QWidget *parent = Q_NULLPTR);
	~VideoControls();

	//������
	void setProgressDuration(qint64 duration);
	bool isSliderDown();
	void setSliderPosition(int position);
	//����ʱ��
	void setProgressText(const QString& text);
	//����״̬
	void setPlaying();
	//��������
	void setVolume(int value);
	//��ȡ�������ڳ������
	bool getVolumVisible();
	//���ڹ���
	void locateWidgets();
	void closeWidget();
	//MV
	void setMvWidget();

private:
	void initVolumeSlider();

private slots:
	void sltVideoPlayOrPause();
	void sltSetPlayCycle();
	void sltSliderProgressClicked();
	void sltSliderProgressReleased();
	void sltSoundVoiceValue(int value);
	void sltVolumControls();

signals:
	void sigVideoPlayOrPause(bool state);
	void sigNextVideoPlay();
	void sigSetPlayCycle(bool state);
	void sigSetPosition(int value);
	void sigSoundVoiceValue(int value);

private:
	Ui::controls ui;
	//����������
	QSharedPointer <VSliderWidget> m_volumeSlider = nullptr;
};