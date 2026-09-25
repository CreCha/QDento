#include "PatientCardPrinter.h"

#include <QString>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QBuffer>
#include <QByteArray>
#include <QDate>
#include <QUrl>
#include <QDesktopServices>
#include <QStandardPaths>
#include <QStringConverter>

#include "Model/Patient.h"
#include "Model/User.h"
#include "Database/DbProcedure.h"
#include "View/CustomImages.h"
#include "PatientExtra.h"

static QString h(const std::string& s) { return QString::fromStdString(s).toHtmlEscaped(); }
static QString u(const char* s) { return QString::fromUtf8(s); }

static QString row(const QString& label, const QString& value)
{
	if (value.trimmed().isEmpty()) return {};
	return "<tr><th>" + label + "</th><td>" + value + "</td></tr>";
}

static QString logoImgTag()
{
	QPixmap logo = CustomImages::customLogo();
	if (logo.isNull()) return {};

	QByteArray bytes;
	QBuffer buffer(&bytes);
	buffer.open(QIODevice::WriteOnly);
	logo.scaled(240, 240, Qt::KeepAspectRatio, Qt::SmoothTransformation).save(&buffer, "PNG");

	return "<img class='logo' src='data:image/png;base64," + QString::fromLatin1(bytes.toBase64()) + "'/>";
}

// Προαιρετικές γραμμές κεφαλίδας από custom/clinic_info.txt (τηλέφωνο, email, ωράριο κ.λπ.)
static QString clinicInfoLines()
{
	QFile f(QDir(CustomImages::folder()).filePath("clinic_info.txt"));
	if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return {};

	QTextStream in(&f);
	in.setEncoding(QStringConverter::Utf8);

	QString result;
	while (!in.atEnd()) {
		auto line = in.readLine().trimmed();
		if (!line.isEmpty()) result += "<div>" + line.toHtmlEscaped() + "</div>";
	}
	return result;
}

void PatientCardPrinter::print(const Patient& p)
{
	const auto& company = User::company();
	const auto extra = DbPatientExtra::get(p.rowid);

	QString html = u(R"HTML(<!DOCTYPE html>
<html lang="el"><head><meta charset="UTF-8"><title>Καρτέλα ασθενή</title>
<style>
 body { font-family: "Segoe UI", Arial, sans-serif; color:#222; margin:30px; font-size:13px; }
 .header { display:flex; align-items:center; border-bottom:3px solid #167a80; padding-bottom:12px; margin-bottom:18px; }
 .logo { width:90px; height:90px; margin-right:18px; }
 .clinic h1 { margin:0; color:#167a80; font-size:22px; }
 .clinic div { color:#555; }
 h2 { color:#167a80; font-size:16px; border-bottom:1px solid #ccc; padding-bottom:4px; margin-top:22px; }
 table { border-collapse:collapse; width:100%; }
 th, td { text-align:left; padding:5px 8px; border-bottom:1px solid #eee; vertical-align:top; }
 .info th { width:170px; color:#555; font-weight:600; }
 .proc th { background:#eef6f6; }
 .alert { background:#fde8e8; border:1px solid #d33; color:#a00; padding:8px 12px; font-weight:bold; margin:10px 0; }
 .right { text-align:right; }
 .footer { margin-top:30px; color:#888; font-size:11px; text-align:right; }
 @media print { body { margin:10mm; } }
</style></head><body onload="window.print()">)HTML");

	// ---- Κεφαλίδα ιατρείου ----
	html += "<div class='header'>" + logoImgTag() + "<div class='clinic'>";
	html += "<h1>" + (company.name.empty() ? u("Οδοντιατρείο") : h(company.name)) + "</h1>";
	if (!company.address.empty()) html += "<div>" + h(company.address) + "</div>";
	html += clinicInfoLines();
	if (!company.identifier.empty()) html += u("<div>ΑΦΜ: ") + h(company.identifier) + "</div>";
	html += "</div></div>";

	// ---- Στοιχεία ασθενή ----
	html += u("<h2>Καρτέλα ασθενή</h2><table class='info'>");
	html += row(u("Ονοματεπώνυμο"), h(p.firstLastName()));
	html += row(u("Πατρώνυμο"), h(extra.fatherName));
	if (p.birth.year != 1900)
		html += row(u("Ημ. γέννησης"), h(p.birth.toLocalFormat()) + u(" (") + QString::number(p.getAge()) + u(" ετών)"));
	html += row(u("Φύλο"), p.sex == Patient::Female ? u("Γυναίκα") : u("Άνδρας"));
	html += row(u("ΑΜΚΑ"), h(p.id));
	html += row(u("ΑΦΜ / ΔΟΥ"), h(extra.afm) + (extra.doy.empty() ? "" : " / " + h(extra.doy)));
	html += row(u("Τηλέφωνο"), h(p.phone));
	html += row(u("Κινητό"), h(extra.mobile));
	html += row(u("Email"), h(extra.email));
	html += row(u("Διεύθυνση"), h(p.address));
	html += "</table>";

	// ---- Ιατρικό ιστορικό ----
	html += u("<h2>Ιατρικό ιστορικό</h2>");
	if (extra.hasAlert()) html += "<div class='alert'>&#9888; " + h(extra.alertText()) + "</div>";

	if (extra.updated.empty()) {
		html += u("<p><i>Δεν έχει συμπληρωθεί.</i></p>");
	}
	else {
		html += "<table class='info'>";
		html += row(u("Αλλεργίες"), extra.allergies.empty() ? u("Καμία γνωστή") : h(extra.allergies));
		html += row(u("Φάρμακα"), h(extra.medications));
		html += row(u("Νοσήματα"), h(extra.diseases));
		html += row(u("Αντιπηκτικά"), extra.anticoagulants ? u("Ναι") : u("Όχι"));
		html += row(u("Εγκυμοσύνη / θηλασμός"), extra.pregnancy ? u("Ναι") : u("Όχι"));
		html += row(u("Καπνιστής"), extra.smoker ? u("Ναι") : u("Όχι"));
		html += row(u("Άλλα"), h(extra.other));
		html += row(u("Τελευταία ενημέρωση"),
			QDate::fromString(QString::fromStdString(extra.updated), Qt::ISODate).toString("dd/MM/yyyy"));
		html += "</table>";
	}

	// ---- Πράξεις ----
	auto procedures = DbProcedure::getPatientProcedures(p.rowid);

	html += u("<h2>Ιστορικό θεραπειών</h2>");

	if (procedures.empty()) {
		html += u("<p><i>Δεν υπάρχουν καταχωρημένες πράξεις.</i></p>");
	}
	else {
		html += u("<table class='proc'><tr><th>Ημερομηνία</th><th>Δόντι</th><th>Πράξη</th><th>Διάγνωση</th><th class='right'>Τιμή</th></tr>");

		double total = 0;

		for (auto& proc : procedures) {
			total += proc.price;
			QString name = h(proc.name);
			if (!proc.notes.empty()) name += "<br/><small>" + h(proc.notes) + "</small>";

			html += "<tr><td>" + h(proc.date.toLocalFormat()) + "</td>"
				+ "<td>" + h(proc.getToothString()) + "</td>"
				+ "<td>" + name + "</td>"
				+ "<td>" + h(proc.diagnosis) + "</td>"
				+ "<td class='right'>" + QString::number(proc.price, 'f', 2) + " &euro;</td></tr>";
		}

		html += u("<tr><th colspan='4' class='right'>Σύνολο</th><th class='right'>")
			+ QString::number(total, 'f', 2) + " &euro;</th></tr></table>";
	}

	html += u("<div class='footer'>Εκτύπωση: ") + QDate::currentDate().toString("dd/MM/yyyy") + "</div>";
	html += "</body></html>";

	// ---- Αποθήκευση σε προσωρινό αρχείο και άνοιγμα ----
	QString path = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
		.filePath(QString("patient_card_%1.html").arg(p.rowid));

	QFile file(path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
	file.write(html.toUtf8());
	file.close();

	QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}
