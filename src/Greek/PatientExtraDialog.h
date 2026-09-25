#pragma once
#include <QDialog>
#include "PatientExtra.h"

class QLineEdit;
class QPlainTextEdit;
class QCheckBox;

// Διάλογος "Στοιχεία & ιατρικό ιστορικό" ασθενή
class PatientExtraDialog : public QDialog
{
public:
	PatientExtraDialog(const PatientExtra& data, const QString& patientName, QWidget* parent = nullptr);
	PatientExtra result() const;

private:
	PatientExtra m_data;

	QLineEdit* fatherEdit;
	QLineEdit* afmEdit;
	QLineEdit* doyEdit;
	QLineEdit* mobileEdit;
	QLineEdit* emailEdit;
	QCheckBox* smsCheck;

	QPlainTextEdit* allergiesEdit;
	QPlainTextEdit* medicationsEdit;
	QPlainTextEdit* diseasesEdit;
	QPlainTextEdit* otherEdit;
	QCheckBox* anticoagCheck;
	QCheckBox* pregnancyCheck;
	QCheckBox* smokerCheck;
};
