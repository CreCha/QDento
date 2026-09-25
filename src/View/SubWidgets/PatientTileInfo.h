#pragma once

#include <QWidget>

#include "ui_PatientTileInfo.h"
#include "View/uiComponents/RoundedFrame.h"
#include "Model/Patient.h"

class QLabel;
class QPushButton;

struct PatientInfoPresenter;

class PatientTileInfo : public RoundedFrame
{
	Q_OBJECT

	PatientInfoPresenter* presenter{ nullptr };
    QMenu* context_menu;

public:
	PatientTileInfo(QWidget *parent = nullptr);
	void setPatient(const Patient& p, int age);
	void setPresenter(PatientInfoPresenter* p) { presenter = p; }

	~PatientTileInfo();

private:
	Ui::PatientTileInfoClass ui;

	// Ελληνικές προσθήκες: ειδοποίηση υγείας, ιστορικό, εκτύπωση
	QLabel* alertLabel{ nullptr };
	QPushButton* historyButton{ nullptr };
	QPushButton* printButton{ nullptr };
	Patient m_patient;
	int m_age{ 0 };
	void openMedicalHistory();
	void refreshAlert();
};
