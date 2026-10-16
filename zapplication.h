#pragma once

#include "single_inst/qtsingleapplication.h"

class QSessionManager;

class zApplication : public QtSingleApplication
{
    Q_OBJECT

public:
    explicit zApplication(int &argc, char **argv, bool guiEnabled = true);
    zApplication(const QString &id, int &argc, char **argv);
    ~zApplication() override = default;

    // Prevent copying
    zApplication(const zApplication &) = delete;
    zApplication &operator=(const zApplication &) = delete;

private slots:
    void onCommitDataRequest(QSessionManager &manager);
};
