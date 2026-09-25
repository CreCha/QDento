#include "SmsReminderDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateEdit>
#include <QTableWidget>
#include <QHeaderView>
#include <QPlainTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QTimer>
#include <QUrl>
#include <QApplication>
#include <QDesktopServices>
#include <QRegularExpression>

#include "Database/Database.h"
#include "Database/DbAppointment.h"
#include "Database/DbPatient.h"
#include "Model/User.h"
#include "View/CustomImages.h"
#include "PatientExtra.h"

static QString u(const char* s) { return QString::fromUtf8(s); }

// ------------------------------------------------------------
// Αρχείο καταγραφής αποστολών (για να μη σταλεί δεύτερη φορά)
// ------------------------------------------------------------
void SmsLog::ensureTable()
{
	static bool done = false;
	if (done) return;
	Db db;
	db.execute("CREATE TABLE IF NOT EXISTS sms_log (appointment_rowid INTEGER, phone TEXT, sent TEXT DEFAULT (datetime('now','localtime')))");
	done = true;
}

bool SmsLog::wasSent(long long appointmentRowid)
{
	ensureTable();
	Db db("SELECT COUNT(*) FROM sms_log WHERE appointment_rowid=?");
	db.bind(1, appointmentRowid);
	int count = 0;
	while (db.hasRows()) count = db.asInt(0);
	return count > 0;
}

void SmsLog::markSent(long long appointmentRowid, const std::string& phone)
{
	ensureTable();
	Db db("INSERT INTO sms_log (appointment_rowid, phone) VALUES (?,?)");
	db.bind(1, appointmentRowid);
	db.bind(2, phone);
	db.execute();
}

// ------------------------------------------------------------
// Βοηθητικά
// ------------------------------------------------------------
static QString configPath()
{
	return QDir(CustomImages::folder()).filePath("sms.json");
}

static const char* defaultConfig = R"JSON({
  "_odhgies": "Βάλτε το API key του παρόχου στο headers. Όσο test_mode είναι true ΔΕΝ στέλνεται τίποτα - μόνο δοκιμή.",
  "test_mode": true,

  "url": "https://api.sms.to/sms/send",
  "headers": {
    "Authorization": "Bearer ΒΑΛΤΕ_ΕΔΩ_ΤΟ_API_KEY",
    "Content-Type": "application/json"
  },
  "body": "{\"to\":\"{phone_intl}\",\"message\":\"{text_json}\",\"sender_id\":\"{sender}\"}",

  "sender": "BATSIOU",
  "country_code": "30",

  "message": "Υπενθύμιση ραντεβού: {date} στις {time}, {clinic}. Για αλλαγή: {clinic_phone}",
  "clinic": "Οδοντιατρείο Μπάτσιου",
  "clinic_phone": "2313045688",

  "_paradeigma_yuboto": {
    "url": "https://api.yuboto.com/omni/v1/Send",
    "headers": { "Authorization": "Basic ΤΟ_API_KEY_ΣΕ_BASE64", "Content-Type": "application/json" },
    "body": "{\"phonenumbers\":[\"{phone_digits}\"],\"sms\":{\"sender\":\"{sender}\",\"text\":\"{text_json}\",\"validity\":180,\"typesms\":\"sms\"}}"
  }
}
)JSON";

// Κανονικοποίηση κινητού σε 10 ψηφία (69xxxxxxxx). Κενό αν δεν είναι έγκυρο.
static QString normalizeMobile(const std::string& raw)
{
	QString d = QString::fromStdString(raw);
	d.remove(QRegularExpression("[^0-9]"));
	if (d.startsWith("0030")) d = d.mid(4);
	else if (d.startsWith("30") && d.size() == 12) d = d.mid(2);
	if (d.size() == 10 && d.startsWith("69")) return d;
	return {};
}

static QString jsonEscape(const QString& s)
{
	QString json = QString::fromUtf8(QJsonDocument(QJsonArray{ s }).toJson(QJsonDocument::Compact));
	return json.mid(2, json.size() - 4); // αφαίρεση [" και "]
}

// ------------------------------------------------------------
// Διάλογος
// ------------------------------------------------------------
SmsReminderDialog::SmsReminderDialog(QWidget* parent) : QDialog(parent)
{
	setWindowTitle(u("Υπενθυμίσεις ραντεβού με SMS"));
	resize(720, 560);

	auto main = new QVBoxLayout(this);

	auto top = new QHBoxLayout();
	top->addWidget(new QLabel(u("Ραντεβού της ημέρας:"), this));
	dateEdit = new QDateEdit(QDate::currentDate().addDays(1), this);
	dateEdit->setCalendarPopup(true);
	dateEdit->setLocale(QLocale(QLocale::Greek, QLocale::Greece));
	dateEdit->setDisplayFormat("dddd dd/MM/yyyy");
	top->addWidget(dateEdit);
	top->addStretch();
	auto settingsButton = new QPushButton(u("Ρυθμίσεις παρόχου"), this);
	top->addWidget(settingsButton);
	main->addLayout(top);

	infoLabel = new QLabel(this);
	infoLabel->setWordWrap(true);
	main->addWidget(infoLabel);

	table = new QTableWidget(0, 4, this);
	table->setHorizontalHeaderLabels({ u("Ώρα"), u("Ασθενής"), u("Κινητό"), u("Κατάσταση") });
	table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
	table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
	table->verticalHeader()->hide();
	table->setSelectionMode(QAbstractItemView::NoSelection);
	table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	main->addWidget(table, 1);

	main->addWidget(new QLabel(u("Μήνυμα (πεδία: {name} {date} {time} {clinic} {clinic_phone}):"), this));
	messageEdit = new QPlainTextEdit(this);
	messageEdit->setFixedHeight(70);
	main->addWidget(messageEdit);

	auto bottom = new QHBoxLayout();
	countLabel = new QLabel(this);
	countLabel->setStyleSheet("color: gray;");
	bottom->addWidget(countLabel, 1);
	sendButton = new QPushButton(u("Αποστολή στους επιλεγμένους"), this);
	auto closeButton = new QPushButton(u("Κλείσιμο"), this);
	bottom->addWidget(sendButton);
	bottom->addWidget(closeButton);
	main->addLayout(bottom);

	connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);
	connect(dateEdit, &QDateEdit::dateChanged, this, [this] { loadAppointments(); });
	connect(messageEdit, &QPlainTextEdit::textChanged, this, [this] { updateCount(); });
	connect(sendButton, &QPushButton::clicked, this, [this] { sendSelected(); });
	connect(settingsButton, &QPushButton::clicked, this, [this] {
		loadConfig(); // δημιουργεί το αρχείο αν λείπει
		QDesktopServices::openUrl(QUrl::fromLocalFile(configPath()));
		QMessageBox::information(this, windowTitle(),
			u("Άνοιξε το αρχείο ρυθμίσεων:\n") + configPath() +
			u("\n\nΜετά την αλλαγή, αποθηκεύστε το και ανοίξτε ξανά αυτό το παράθυρο."));
	});

	loadConfig();
	messageEdit->setPlainText(m_config.value("message").toString());
	loadAppointments();
}

bool SmsReminderDialog::loadConfig()
{
	QFile f(configPath());

	if (!f.exists()) {
		if (f.open(QIODevice::WriteOnly)) {
			f.write(defaultConfig);
			f.close();
		}
	}

	if (!f.open(QIODevice::ReadOnly)) return false;

	QJsonParseError err;
	auto doc = QJsonDocument::fromJson(f.readAll(), &err);

	if (err.error != QJsonParseError::NoError) {
		QMessageBox::warning(this, windowTitle(),
			u("Σφάλμα στο αρχείο sms.json: ") + err.errorString());
		m_config = QJsonDocument::fromJson(defaultConfig).object();
		return false;
	}

	m_config = doc.object();

	bool test = m_config.value("test_mode").toBool(true);
	infoLabel->setText(test
		? u("<b style='color:#b36b00'>ΔΟΚΙΜΑΣΤΙΚΗ ΛΕΙΤΟΥΡΓΙΑ</b> – δεν στέλνονται πραγματικά μηνύματα. "
			"Ρυθμίστε τον πάροχο και αλλάξτε το test_mode σε false.")
		: u("Επιλέξτε ασθενείς και πατήστε Αποστολή. Αποστέλλονται μόνο όσοι έχουν δώσει συναίνεση."));

	return true;
}

void SmsReminderDialog::loadAppointments()
{
	m_rows.clear();

	QDate day = dateEdit->date();
	auto events = DbAppointment::get(day, day, User::dentist().rowID);

	std::sort(events.begin(), events.end(), [](auto& a, auto& b) { return a.start < b.start; });

	for (auto& e : events) {

		if (e.start.date() != day) continue;

		Row r;
		r.appointmentRowid = e.rowid;
		r.time = e.start.time().toString("HH:mm");

		if (e.patient_rowid) {
			auto p = DbPatient::get(e.patient_rowid);
			auto extra = DbPatientExtra::get(e.patient_rowid);
			r.patientName = QString::fromStdString(p.firstLastName());
			r.phone = normalizeMobile(extra.mobile);
			if (r.phone.isEmpty()) r.phone = normalizeMobile(p.phone);
			r.consent = extra.smsConsent;
		}
		else {
			r.patientName = QString::fromStdString(e.summary);
		}

		r.alreadySent = SmsLog::wasSent(e.rowid);
		m_rows.push_back(r);
	}

	table->setRowCount((int)m_rows.size());

	for (int i = 0; i < (int)m_rows.size(); i++) {

		auto& r = m_rows[i];

		auto timeItem = new QTableWidgetItem(r.time);
		timeItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);

		QString status;
		bool ok = false;

		if (r.phone.isEmpty()) status = u("Χωρίς κινητό");
		else if (!r.consent) status = u("Χωρίς συναίνεση SMS");
		else if (r.alreadySent) status = u("Έχει ήδη σταλεί");
		else { status = u("Έτοιμο"); ok = true; }

		timeItem->setCheckState(ok ? Qt::Checked : Qt::Unchecked);
		if (r.phone.isEmpty() || !r.consent) timeItem->setFlags(Qt::ItemIsEnabled);

		table->setItem(i, 0, timeItem);
		table->setItem(i, 1, new QTableWidgetItem(r.patientName));
		table->setItem(i, 2, new QTableWidgetItem(r.phone));
		table->setItem(i, 3, new QTableWidgetItem(status));
	}

	if (m_rows.empty()) {
		countLabel->setText(u("Δεν υπάρχουν ραντεβού αυτή την ημέρα."));
	}
	else updateCount();
}

QString SmsReminderDialog::buildMessage(const Row& r) const
{
	QString text = messageEdit->toPlainText().trimmed();
	QLocale gr(QLocale::Greek, QLocale::Greece);

	text.replace("{name}", r.patientName);
	text.replace("{date}", gr.toString(dateEdit->date(), "dddd dd/MM"));
	text.replace("{time}", r.time);
	text.replace("{clinic}", m_config.value("clinic").toString());
	text.replace("{clinic_phone}", m_config.value("clinic_phone").toString());

	return text;
}

void SmsReminderDialog::updateCount()
{
	if (m_rows.empty()) return;

	auto text = buildMessage(m_rows.front());
	int len = text.size();
	int parts = len <= 70 ? 1 : (len + 66) / 67; // ελληνικά = Unicode SMS

	countLabel->setText(u("Χαρακτήρες: %1 (%2 SMS ανά ασθενή)").arg(len).arg(parts));
}

bool SmsReminderDialog::sendOne(const Row& r, const QString& text, QString& error)
{
	QString cc = m_config.value("country_code").toString("30");

	QString body = m_config.value("body").toString();
	body.replace("{phone_intl}", "+" + cc + r.phone);
	body.replace("{phone_digits}", cc + r.phone);
	body.replace("{phone}", r.phone);
	body.replace("{sender}", m_config.value("sender").toString());
	body.replace("{text_json}", jsonEscape(text));
	body.replace("{text_url}", QString::fromLatin1(QUrl::toPercentEncoding(text)));

	QNetworkRequest req(QUrl(m_config.value("url").toString()));

	auto headers = m_config.value("headers").toObject();
	for (auto it = headers.begin(); it != headers.end(); ++it)
		req.setRawHeader(it.key().toUtf8(), it.value().toString().toUtf8());

	static QNetworkAccessManager manager;
	QNetworkReply* reply = manager.post(req, body.toUtf8());

	QEventLoop loop;
	QTimer timer;
	timer.setSingleShot(true);
	QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
	QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
	timer.start(20000);
	loop.exec();

	if (!reply->isFinished()) {
		reply->abort();
		reply->deleteLater();
		error = u("Λήξη χρόνου σύνδεσης");
		return false;
	}

	int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	QByteArray response = reply->readAll();
	bool ok = reply->error() == QNetworkReply::NoError && status >= 200 && status < 300;

	if (!ok) error = status
		? QString("HTTP %1: %2").arg(status).arg(QString::fromUtf8(response.left(150)))
		: reply->errorString();

	reply->deleteLater();
	return ok;
}

void SmsReminderDialog::sendSelected()
{
	std::vector<int> selected;

	for (int i = 0; i < (int)m_rows.size(); i++)
		if (table->item(i, 0)->checkState() == Qt::Checked) selected.push_back(i);

	if (selected.empty()) {
		QMessageBox::information(this, windowTitle(), u("Δεν έχει επιλεγεί κανένας ασθενής."));
		return;
	}

	bool test = m_config.value("test_mode").toBool(true);

	if (test) {
		QString preview = u("ΔΟΚΙΜΗ – δεν στάλθηκε τίποτα. Θα στέλνονταν:\n\n");
		for (int i : selected)
			preview += m_rows[i].phone + ": " + buildMessage(m_rows[i]) + "\n\n";
		QMessageBox::information(this, windowTitle(), preview);
		return;
	}

	if (QMessageBox::question(this, windowTitle(),
		u("Αποστολή %1 SMS;").arg(selected.size())) != QMessageBox::Yes) return;

	sendButton->setEnabled(false);
	int sent = 0;

	for (int i : selected) {

		auto& r = m_rows[i];
		table->item(i, 3)->setText(u("Αποστολή..."));
		QApplication::processEvents();

		QString error;

		if (sendOne(r, buildMessage(r), error)) {
			SmsLog::markSent(r.appointmentRowid, r.phone.toStdString());
			r.alreadySent = true;
			table->item(i, 3)->setText(u("Εστάλη ✓"));
			table->item(i, 0)->setCheckState(Qt::Unchecked);
			sent++;
		}
		else {
			table->item(i, 3)->setText(u("Σφάλμα: ") + error);
		}

		QApplication::processEvents();
	}

	sendButton->setEnabled(true);

	QMessageBox::information(this, windowTitle(),
		u("Στάλθηκαν %1 από %2 μηνύματα.").arg(sent).arg(selected.size()));
}
