#include "zapplication.h"
#include "mainwindow.h"
#include "settings.h"

int main(int argc, char **argv)
{
    zApplication app(argc, argv);

    settings.load();

    // Check for an already running instance
    if (settings.getSingleInstance() && app.isRunning()) {
        app.sendMessage("Only one instance is allowed");
        return 0; // Exit cleanly as the secondary instance
    }

    app.setQuitOnLastWindowClosed(false);

    MainWindow w;
    app.setActivationWindow(&w);

    return app.exec();
}
