#include "PatientExtra.h"
#include "Database/Database.h"

std::string PatientExtra::alertText() const
{
	std::string result;

	auto add = [&](const std::string& s) {
		if (!result.empty()) result += "  |  ";
		result += s;
	};

	if (!allergies.empty()) add("ΑΛΛΕΡΓΙΑ: " + allergies);
	if (anticoagulants) add("ΑΝΤΙΠΗΚΤΙΚΑ");
	if (pregnancy) add("ΕΓΚΥΜΟΣΥΝΗ");

	return result;
}

void DbPatientExtra::ensureTable()
{
	static bool done = false;
	if (done) return;

	Db db;
	db.execute(
		"CREATE TABLE IF NOT EXISTS patient_gr ("
		"patient_rowid INTEGER PRIMARY KEY REFERENCES patient (rowid) ON DELETE CASCADE ON UPDATE CASCADE, "
		"father_name TEXT, afm TEXT, doy TEXT, mobile TEXT, email TEXT, sms_consent INTEGER DEFAULT 0, "
		"allergies TEXT, medications TEXT, diseases TEXT, other TEXT, "
		"anticoagulants INTEGER DEFAULT 0, pregnancy INTEGER DEFAULT 0, smoker INTEGER DEFAULT 0, "
		"updated TEXT)"
	);

	done = true;
}

PatientExtra DbPatientExtra::get(long long patientRowid)
{
	ensureTable();

	PatientExtra e;
	e.patientRowid = patientRowid;

	if (patientRowid <= 0) return e;

	Db db(
		"SELECT father_name, afm, doy, mobile, email, sms_consent, "
		"allergies, medications, diseases, other, anticoagulants, pregnancy, smoker, updated "
		"FROM patient_gr WHERE patient_rowid=?"
	);

	db.bind(1, patientRowid);

	while (db.hasRows())
	{
		e.fatherName = db.asString(0);
		e.afm = db.asString(1);
		e.doy = db.asString(2);
		e.mobile = db.asString(3);
		e.email = db.asString(4);
		e.smsConsent = db.asInt(5);
		e.allergies = db.asString(6);
		e.medications = db.asString(7);
		e.diseases = db.asString(8);
		e.other = db.asString(9);
		e.anticoagulants = db.asInt(10);
		e.pregnancy = db.asInt(11);
		e.smoker = db.asInt(12);
		e.updated = db.asString(13);
	}

	return e;
}

bool DbPatientExtra::save(const PatientExtra& e)
{
	ensureTable();

	if (e.patientRowid <= 0) return false;

	Db db(
		"INSERT OR REPLACE INTO patient_gr "
		"(patient_rowid, father_name, afm, doy, mobile, email, sms_consent, "
		"allergies, medications, diseases, other, anticoagulants, pregnancy, smoker, updated) "
		"VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"
	);

	db.bind(1, e.patientRowid);
	db.bind(2, e.fatherName);
	db.bind(3, e.afm);
	db.bind(4, e.doy);
	db.bind(5, e.mobile);
	db.bind(6, e.email);
	db.bind(7, e.smsConsent ? 1 : 0);
	db.bind(8, e.allergies);
	db.bind(9, e.medications);
	db.bind(10, e.diseases);
	db.bind(11, e.other);
	db.bind(12, e.anticoagulants ? 1 : 0);
	db.bind(13, e.pregnancy ? 1 : 0);
	db.bind(14, e.smoker ? 1 : 0);
	db.bind(15, e.updated);

	return db.execute();
}
