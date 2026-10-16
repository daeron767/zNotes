#include "zapplication.h"
#include "settings.h"

#include <QSessionManager>

zApplication::zApplication(int &argc, char **argv, bool guiEnabled)
    : QtSingleApplication(argc, argv, guiEnabled)
{
    connect(this, &QGuiApplication::commitDataRequest,
            this, &zApplication::onCommitDataRequest,
            Qt::DirectConnection);
}

zApplication::zApplication(const QString &id, int &argc, char **argv)
    : QtSingleApplication(id, argc, argv)
{
    connect(this, &QGuiApplication::commitDataRequest,
            this, &zApplication::onCommitDataRequest,
            Qt::DirectConnection);
}

void zApplication::onCommitDataRequest(QSessionManager & /*manager*/)
{
    // Save settings during session shutdown (e.g., KDE/GNOME logout)
    settings.save();
}
