#include <QApplication>

#include "installerwindow.h"

int main(int argc, char *argv[]) {
    QApplication application(argc, argv);
    QApplication::setApplicationName(QStringLiteral("AnythingLLM Installer"));
    QApplication::setOrganizationName(QStringLiteral("Mintplex Labs"));
    QApplication::setApplicationVersion(QStringLiteral(APP_VERSION));

    InstallerWindow window;
    window.show();

    return application.exec();
}
