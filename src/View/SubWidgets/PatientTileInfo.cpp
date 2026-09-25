#include "PatientTileInfo.h"

#include <QMenu>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>

#include "Presenter/PatientInfoPresenter.h"
#include "View/Theme.h"
#include "Greek/PatientExtra.h"
#include "Greek/PatientExtraDialog.h"
#include "Greek/PatientCardPrinter.h"

PatientTileInfo::PatientTileInfo(QWidget *parent)
	: RoundedFrame(parent)
{
	ui.setupUi(this);

	setFrameColor(Theme::border);

    //init context menu
    context_menu = new QMenu(this);

    QAction* action;

    action = (new QAction(tr("Edit"), context_menu));
    connect(action, &QAction::triggered, this, [=, this] { ui.patientTile->click(); });

    action->setIcon(QIcon(":/icons/icon_edit.png"));
    context_menu->addAction(action);

    action = (new QAction(tr("New Dental Visit"), context_menu));
    connect(action, &QAction::triggered, this, [=, this] { presenter->openDocument(TabType::DentalVisit); });
    action->setIcon(QIcon(":/icons/icon_sheet.png"));
    context_menu->addAction(action);

    action = (new QAction(tr("New Periodontal Measurment"), context_menu));
    connect(action, &QAction::triggered, this, [=, this] { presenter->openDocument(TabType::PerioStatus); });
    action->setIcon(QIcon(":/icons/icon_periosheet.png"));
    context_menu->addAction(action);

    action = (new QAction(tr("Schedule and Appointment"), context_menu));
    connect(action, &QAction::triggered, this, [=, this] { presenter->openDocument(TabType::Calendar); });
    action->setIcon(QIcon(":/icons/icon_calendar.png"));
    context_menu->addAction(action);

    action = (new QAction(tr("Patient History"), context_menu));
    connect(action, &QAction::triggered, this, [=, this] { presenter->openDocument(TabType::PatientSummary); });
    action->setIcon(QIcon(":/icons/icon_history.png"));
    context_menu->addAction(action);

    action = (new QAction(QString::fromUtf8("Στοιχεία & ιατρικό ιστορικό"), context_menu));
    connect(action, &QAction::triggered, this, [=, this] { openMedicalHistory(); });
    action->setIcon(QIcon(":/icons/icon_edit.png"));
    context_menu->addAction(action);

    action = (new QAction(QString::fromUtf8("Εκτύπωση καρτέλας ασθενή"), context_menu));
    connect(action, &QAction::triggered, this, [=, this] { if (m_patient.rowid) PatientCardPrinter::print(m_patient); });
    action->setIcon(QIcon(":/icons/icon_print.png"));
    context_menu->addAction(action);

    context_menu->setStyleSheet(Theme::getPopupMenuStylesheet());

    // Μπάρα κάτω από την καρτέλα: ειδοποίηση υγείας + κουμπιά
    auto bar = new QWidget(this);
    auto barLayout = new QHBoxLayout(bar);
    barLayout->setContentsMargins(10, 4, 10, 6);
    barLayout->setSpacing(6);

    alertLabel = new QLabel(bar);
    alertLabel->setStyleSheet("color: #b00020; font-weight: bold;");
    alertLabel->setTextInteractionFlags(Qt::NoTextInteraction);

    historyButton = new QPushButton(QString::fromUtf8("Ιατρικό ιστορικό"), bar);
    historyButton->setCursor(Qt::PointingHandCursor);
    historyButton->setToolTip(QString::fromUtf8("Πατρώνυμο, ΑΦΜ, κινητό, αλλεργίες, φάρμακα, νοσήματα"));

    printButton = new QPushButton(QString::fromUtf8("Εκτύπωση"), bar);
    printButton->setCursor(Qt::PointingHandCursor);
    printButton->setToolTip(QString::fromUtf8("Εκτύπωση καρτέλας ασθενή με τα στοιχεία του ιατρείου"));

    barLayout->addWidget(alertLabel, 1);
    barLayout->addWidget(historyButton);
    barLayout->addWidget(printButton);

    ui.verticalLayout->addWidget(bar);

    connect(historyButton, &QPushButton::clicked, this, [=, this] { openMedicalHistory(); });
    connect(printButton, &QPushButton::clicked, this, [=, this] { if (m_patient.rowid) PatientCardPrinter::print(m_patient); });

    //connect signalsh

    connect(ui.patientTile, &QPushButton::clicked, this, [=, this] {
		if (presenter) presenter->patientTileClicked();
	});

	connect (ui.patientTile->notesButton, &QPushButton::clicked, this, [=, this] {
		if (presenter) presenter->notesRequested();
	});

	connect (ui.patientTile->appointmentButton, &QPushButton::clicked, this, [=, this] {
		if (presenter) presenter->appointmentClicked();
	});

    connect (ui.patientTile->notificationButton, &QPushButton::clicked, this, [=, this]{
        if (presenter) presenter->notificationClicked();
    });

    connect(ui.patientTile, &TileButton::customContextMenuRequested, this, [&](QPoint point) {
        context_menu->popup(point);
    });

}

void PatientTileInfo::setPatient(const Patient& p, int age)
{
	ui.patientTile->setData(p, age);

	m_patient = p;
	m_age = age;
	refreshAlert();
}

void PatientTileInfo::refreshAlert()
{
	auto extra = DbPatientExtra::get(m_patient.rowid);

	if (extra.hasAlert()) {
		auto text = QString::fromStdString(extra.alertText());
		alertLabel->setToolTip(text);
		alertLabel->setText(QString(QChar(0x26A0)) + " " + alertLabel->fontMetrics().elidedText(text, Qt::ElideRight, 320));
		historyButton->setStyleSheet("");
	}
	else if (extra.updated.empty()) {
		alertLabel->setText(QString::fromUtf8("Χωρίς ιατρικό ιστορικό"));
		alertLabel->setToolTip("");
		alertLabel->setStyleSheet("color: gray;");
		return;
	}
	else {
		alertLabel->setText("");
		alertLabel->setToolTip("");
	}

	alertLabel->setStyleSheet("color: #b00020; font-weight: bold;");
}

void PatientTileInfo::openMedicalHistory()
{
	if (!m_patient.rowid) return;

	PatientExtraDialog d(
		DbPatientExtra::get(m_patient.rowid),
		QString::fromStdString(m_patient.firstLastName()),
		this
	);

	if (d.exec() != QDialog::Accepted) return;

	DbPatientExtra::save(d.result());
	refreshAlert();
}

PatientTileInfo::~PatientTileInfo()
{}
