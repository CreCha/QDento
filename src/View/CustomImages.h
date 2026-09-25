#pragma once
// Προσαρμοσμένες εικόνες ιατρείου.
// Το πρόγραμμα κοιτάει στον φάκελο %APPDATA%\QDento\custom για:
//   logo.png        -> λογότυπο αρχικής οθόνης
//   splash.png      -> εικόνα εκκίνησης
//   background.jpg  -> φωτογραφία φόντου αρχικής οθόνης (ή background.png)
// Αν κάποιο αρχείο λείπει, χρησιμοποιείται η ενσωματωμένη εικόνα του QDento.

#include <QPixmap>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

namespace CustomImages
{
    inline QString folder()
    {
        QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
        if (!dir.exists("custom")) dir.mkpath("custom");
        return dir.filePath("custom");
    }

    // Επιστρέφει την πρώτη εικόνα που υπάρχει από τα ονόματα, αλλιώς κενό QPixmap
    inline QPixmap load(const QStringList& names)
    {
        const QDir dir(folder());
        for (const auto& n : names) {
            const QString path = dir.filePath(n);
            if (QFileInfo::exists(path)) {
                QPixmap p(path);
                if (!p.isNull()) return p;
            }
        }
        return QPixmap();
    }

    inline QPixmap customLogo()   { return load({ "logo.png", "logo.jpg" }); }

    inline QPixmap logo()
    {
        auto p = customLogo();
        return p.isNull() ? QPixmap(":/icons/qDento.png") : p;
    }

    inline QPixmap splash()
    {
        auto p = load({ "splash.png", "splash.jpg" });
        return p.isNull() ? QPixmap(":/other/splash.png") : p;
    }

    inline QPixmap background()
    {
        return load({ "background.jpg", "background.png", "background.jpeg" });
    }
}
