#pragma once
struct Patient;

// Εκτύπωση καρτέλας ασθενή με τα στοιχεία του ιατρείου.
// Δημιουργεί HTML και το ανοίγει στον browser με αυτόματο παράθυρο εκτύπωσης.
namespace PatientCardPrinter
{
	void print(const Patient& patient);
}
