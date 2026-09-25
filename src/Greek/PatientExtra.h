#pragma once
// Επιπλέον στοιχεία ασθενή για ελληνικό ιατρείο + ιατρικό ιστορικό (αναμνηστικό)
#include <string>

struct PatientExtra
{
	long long patientRowid{ 0 };

	// Στοιχεία
	std::string fatherName;
	std::string afm;
	std::string doy;
	std::string mobile;
	std::string email;
	bool smsConsent{ false };

	// Ιατρικό ιστορικό
	std::string allergies;
	std::string medications;
	std::string diseases;
	std::string other;
	bool anticoagulants{ false };
	bool pregnancy{ false };
	bool smoker{ false };
	std::string updated; // ημερομηνία τελευταίας ενημέρωσης (ISO)

	bool hasAlert() const { return !allergies.empty() || anticoagulants || pregnancy; }
	std::string alertText() const;
};

namespace DbPatientExtra
{
	void ensureTable();
	PatientExtra get(long long patientRowid);
	bool save(const PatientExtra& e);
}
