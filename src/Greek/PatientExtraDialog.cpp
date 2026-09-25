#include "PatientExtraDialog.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QTabWidget>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QCheckBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QRegularExpressionValidator>
#include <QMessageBox>
#include <QDate>

static QString q(const std::string& s) { return QString::fromStdString(s); }
static std::string s(const QString& q) { return q.trimmed().toStdString(); }

static QPlainTextEdit* textBox(const std::string& value, QWidget* parent)
{
	auto e = new QPlainTextEdit(q(value), parent);
	e->setTabChangesFocus(true);
	e->setFixedHeight(60);
	return e;
}

PatientExtraDialog::PatientExtraDialog(const PatientExtra& data, const QString& patientName, QWidget* parent)
	: QDialog(parent), m_data(data)
{
	setWindowTitle(QString::fromUtf8("Στοιχεία & ιατρικό ιστορικό - ") + patientName);
	setMinimumWidth(520);

	auto main = new QVBoxLayout(this);
	auto tabs = new QTabWidget(this);
	main->addWidget(tabs);

	// ---------- Καρτέλα: Στοιχεία ----------
	auto infoPage = new QWidget(tabs);
	auto infoForm = new QFormLayout(infoPage);

	fatherEdit = new QLineEdit(q(data.fatherName), infoPage);
	afmEdit = new QLineEdit(q(data.afm), infoPage);
	afmEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]{0,9}"), afmEdit));
	afmEdit->setPlaceholderText(QString::fromUtf8("9 ψηφία"));
	doyEdit = new QLineEdit(q(data.doy), infoPage);
	mobileEdit = new QLineEdit(q(data.mobile), infoPage);
	mobileEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9+ ]{0,15}"), mobileEdit));
	mobileEdit->setPlaceholderText("69xxxxxxxx");
	emailEdit = new QLineEdit(q(data.email), infoPage);
	smsCheck = new QCheckBox(QString::fromUtf8("Συναινεί σε υπενθυμίσεις ραντεβού με SMS"), infoPage);
	smsCheck->setChecked(data.smsConsent);

	infoForm->addRow(QString::fromUtf8("Πατρώνυμο:"), fatherEdit);
	infoForm->addRow(QString::fromUtf8("ΑΦΜ:"), afmEdit);
	infoForm->addRow(QString::fromUtf8("ΔΟΥ:"), doyEdit);
	infoForm->addRow(QString::fromUtf8("Κινητό:"), mobileEdit);
	infoForm->addRow(QString::fromUtf8("Email:"), emailEdit);
	infoForm->addRow("", smsCheck);

	tabs->addTab(infoPage, QString::fromUtf8("Στοιχεία"));

	// ---------- Καρτέλα: Ιατρικό ιστορικό ----------
	auto medPage = new QWidget(tabs);
	auto medForm = new QFormLayout(medPage);

	allergiesEdit = textBox(data.allergies, medPage);
	allergiesEdit->setPlaceholderText(QString::fromUtf8("π.χ. πενικιλίνη, λάτεξ, τοπικά αναισθητικά"));
	medicationsEdit = textBox(data.medications, medPage);
	diseasesEdit = textBox(data.diseases, medPage);
	diseasesEdit->setPlaceholderText(QString::fromUtf8("π.χ. διαβήτης, υπέρταση, καρδιοπάθεια"));
	otherEdit = textBox(data.other, medPage);

	anticoagCheck = new QCheckBox(QString::fromUtf8("Λαμβάνει αντιπηκτικά / αντιαιμοπεταλιακά"), medPage);
	anticoagCheck->setChecked(data.anticoagulants);
	pregnancyCheck = new QCheckBox(QString::fromUtf8("Εγκυμοσύνη / θηλασμός"), medPage);
	pregnancyCheck->setChecked(data.pregnancy);
	smokerCheck = new QCheckBox(QString::fromUtf8("Καπνιστής"), medPage);
	smokerCheck->setChecked(data.smoker);

	medForm->addRow(QString::fromUtf8("Αλλεργίες:"), allergiesEdit);
	medForm->addRow(QString::fromUtf8("Φάρμακα:"), medicationsEdit);
	medForm->addRow(QString::fromUtf8("Νοσήματα:"), diseasesEdit);
	medForm->addRow("", anticoagCheck);
	medForm->addRow("", pregnancyCheck);
	medForm->addRow("", smokerCheck);
	medForm->addRow(QString::fromUtf8("Άλλα:"), otherEdit);

	QString updatedText = data.updated.empty()
		? QString::fromUtf8("Δεν έχει συμπληρωθεί ακόμα")
		: QString::fromUtf8("Τελευταία ενημέρωση: ") + QDate::fromString(q(data.updated), Qt::ISODate).toString("dd/MM/yyyy");
	auto updatedLabel = new QLabel(updatedText, medPage);
	updatedLabel->setStyleSheet("color: gray;");
	medForm->addRow("", updatedLabel);

	tabs->addTab(medPage, QString::fromUtf8("Ιατρικό ιστορικό"));

	// Αν υπάρχει ήδη ιστορικό, άνοιξε κατευθείαν εκεί
	if (!data.updated.empty()) tabs->setCurrentIndex(1);

	// ---------- Κουμπιά ----------
	auto buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
	buttons->button(QDialogButtonBox::Save)->setText(QString::fromUtf8("Αποθήκευση"));
	buttons->button(QDialogButtonBox::Cancel)->setText(QString::fromUtf8("Άκυρο"));
	main->addWidget(buttons);

	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(buttons, &QDialogButtonBox::accepted, this, [this] {

		auto afm = afmEdit->text().trimmed();

		if (!afm.isEmpty() && afm.size() != 9) {
			QMessageBox::warning(this, windowTitle(), QString::fromUtf8("Το ΑΦΜ πρέπει να έχει 9 ψηφία."));
			return;
		}

		accept();
	});
}

PatientExtra PatientExtraDialog::result() const
{
	PatientExtra r = m_data;

	r.fatherName = s(fatherEdit->text());
	r.afm = s(afmEdit->text());
	r.doy = s(doyEdit->text());
	r.mobile = s(mobileEdit->text());
	r.email = s(emailEdit->text());
	r.smsConsent = smsCheck->isChecked();

	r.allergies = s(allergiesEdit->toPlainText());
	r.medications = s(medicationsEdit->toPlainText());
	r.diseases = s(diseasesEdit->toPlainText());
	r.other = s(otherEdit->toPlainText());
	r.anticoagulants = anticoagCheck->isChecked();
	r.pregnancy = pregnancyCheck->isChecked();
	r.smoker = smokerCheck->isChecked();
	r.updated = QDate::currentDate().toString(Qt::ISODate).toStdString();

	return r;
}
