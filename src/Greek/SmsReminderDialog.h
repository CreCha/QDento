#pragma once
#include <QDialog>
#include <QJsonObject>
#include <vector>
#include <string>

class QDateEdit;
class QTableWidget;
class QPlainTextEdit;
class QLabel;
class QPushButton;

// Υπενθυμίσεις ραντεβού με SMS.
// Οι ρυθμίσεις του παρόχου βρίσκονται στο %APPDATA%\QDento\custom\sms.json
class SmsReminderDialog : public QDialog
{
public:
	SmsReminderDialog(QWidget* parent = nullptr);

private:
	struct Row {
		long long appointmentRowid{ 0 };
		QString time;
		QString patientName;
		QString phone;          // κανονικοποιημένο κινητό (μόνο ψηφία, χωρίς πρόθεμα)
		bool consent{ false };
		bool alreadySent{ false };
	};

	QJsonObject m_config;
	std::vector<Row> m_rows;

	QDateEdit* dateEdit;
	QTableWidget* table;
	QPlainTextEdit* messageEdit;
	QLabel* infoLabel;
	QLabel* countLabel;
	QPushButton* sendButton;

	bool loadConfig();
	void loadAppointments();
	QString buildMessage(const Row& r) const;
	bool sendOne(const Row& r, const QString& text, QString& error);
	void sendSelected();
	void updateCount();
};

namespace SmsLog
{
	void ensureTable();
	bool wasSent(long long appointmentRowid);
	void markSent(long long appointmentRowid, const std::string& phone);
}
