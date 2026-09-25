#pragma once

#include <QWidget>
#include "ui_WelcomeWidget.h"
#include <QPixmap>
#include <QPaintEvent>

class MainPresenter;

class WelcomeWidget : public QWidget
{
	Q_OBJECT

public:
	WelcomeWidget(QWidget *parent = nullptr);
	~WelcomeWidget();

protected:
	void paintEvent(QPaintEvent* e) override;

private:
	Ui::WelcomeWidgetClass ui;
	QPixmap m_background;
};
