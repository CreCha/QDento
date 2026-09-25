#include "WelcomeWidget.h"
#include "View/Theme.h"
#include "Presenter/MainPresenter.h"
#include <QDesktopServices>
#include <QDate>
#include <QPainter>
#include <QPainterPath>
#include "View/CustomImages.h"

#include "View/Widgets/AboutDialog.h"

WelcomeWidget::WelcomeWidget(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	// Τα παραστατικά εκδίδονται από άλλο πρόγραμμα
	ui.invoiceButton->hide();
	ui.label_3->hide();

    auto date = Date::currentDate();

    // Λογότυπο: του ιατρείου αν υπάρχει, αλλιώς του QDento
    if (!CustomImages::customLogo().isNull()) {
        ui.cornerLabel->setMinimumSize(200, 200);
        ui.cornerLabel->setMaximumSize(200, 200);
    }
    ui.cornerLabel->setPixmap(CustomImages::logo());

    m_background = CustomImages::background();

    if (m_background.isNull()) {
        setStyleSheet("color: " + Theme::colorToString(Theme::fontTurquoise) + "; background-color:" + Theme::colorToString(Theme::background));
    }
    else {
        // Με φωτογραφία φόντου: τα στοιχεία γίνονται διάφανα και το φόντο ζωγραφίζεται στο paintEvent
        setAttribute(Qt::WA_StyledBackground, false);
        setStyleSheet("color: " + Theme::colorToString(Theme::fontTurquoise) + "; background: transparent;");
    }

    ui.ambButton->setIcon(QIcon(":/icons/icon_sheet.png"));
    ui.perioButton->setIcon(QIcon(":/icons/icon_periosheet.png"));
    ui.invoiceButton->setIcon(QIcon(":/icons/icon_invoice.png"));
    ui.browser->setIcon(QIcon(":/icons/icon_open.png"));
    ui.settingsButton->setIcon(QIcon(":/icons/icon_settings.png"));
    ui.calendar->setIcon(QIcon(":/icons/icon_calendar.png"));
    ui.donateButton->setIcon(QIcon(":/icons/icon_donate.png"));
    ui.notifButton->setIcon(QIcon(":/icons/icon_bell.png"));
    ui.aboutButton->setIcon(QIcon(":/icons/icon_question.png"));

    connect(ui.ambButton, &QPushButton::clicked, this, [&] { MainPresenter::get().newAmbPressed(); });
    connect(ui.perioButton, &QPushButton::clicked, this, [&] { MainPresenter::get().newPerioPressed(); });
    connect(ui.invoiceButton, &QPushButton::clicked, this, [&] { MainPresenter::get().newInvoicePressed(); });
    connect(ui.browser, &QPushButton::clicked, this, [&] { MainPresenter::get().showBrowser(); });
    connect(ui.settingsButton, &QPushButton::clicked, this, [&] { MainPresenter::get().settingsPressed(); });
    connect(ui.calendar, &QPushButton::clicked, this, [&] { MainPresenter::get().openCalendar(); });
    connect(ui.donateButton, &QPushButton::clicked, this, [&] { QDesktopServices::openUrl(QUrl("https://www.paypal.com/donate/?hosted_button_id=WJBJECQ247WN6", QUrl::TolerantMode)); });
    connect(ui.notifButton, &QPushButton::clicked, this, [&] { MainPresenter::get().notificationPressed(); });
    connect(ui.aboutButton, &QPushButton::clicked, this, [&] { AboutDialog d; d.exec(); });
}

WelcomeWidget::~WelcomeWidget()
{}

void WelcomeWidget::paintEvent(QPaintEvent* e)
{
    if (m_background.isNull()) {
        QWidget::paintEvent(e);
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.setRenderHint(QPainter::Antialiasing);

    // Φωτογραφία σε όλη την επιφάνεια, με διατήρηση αναλογιών (γεμίζει και κόβει τα άκρα)
    QPixmap scaled = m_background.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    QPoint topLeft((width() - scaled.width()) / 2, (height() - scaled.height()) / 2);
    painter.drawPixmap(topLeft, scaled);

    // Ημιδιάφανο λευκό πάνελ πίσω από τα κουμπιά, για να διαβάζονται οι τίτλοι
    QRect panel = ui.frame->geometry().adjusted(-15, -15, 15, 15);
    QPainterPath path;
    path.addRoundedRect(panel, Theme::radius, Theme::radius);
    painter.fillPath(path, QColor(255, 255, 255, 215));

    // Το ίδιο πίσω από το λογότυπο
    QRect logoRect = ui.cornerLabel->geometry().adjusted(-10, -10, 10, 10);
    QPainterPath logoPath;
    logoPath.addRoundedRect(logoRect, Theme::radius, Theme::radius);
    painter.fillPath(logoPath, QColor(255, 255, 255, 215));
}
