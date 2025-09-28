#ifndef INSTALLERWINDOW_H
#define INSTALLERWINDOW_H

#include <QMainWindow>

#include "installerlogic.h"

class QLabel;
class QLineEdit;
class QPushButton;
class QCheckBox;
class QTextEdit;
class QProgressBar;

class InstallerWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit InstallerWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void handleDetectionFinished(const InstallerLogic::InstallationStatus &status);
    void handleInstallationProgress(const QString &message);
    void handleInstallationStep(int value);
    void handleInstallationFinished(const InstallerLogic::InstallResult &result);
    void startInstallation();
    void browseForPath();
    void triggerDetection();

private:
    void buildUi();
    void setUiEnabled(bool enabled);
    void updateUiForStatus(const InstallerLogic::InstallationStatus &status);
    void appendLogMessage(const QString &message);

    InstallerLogic *m_logic = nullptr;
    InstallerLogic::InstallationStatus m_currentStatus;
    bool m_installationInProgress = false;

    QLabel *m_statusLabel = nullptr;
    QLineEdit *m_pathEdit = nullptr;
    QPushButton *m_installButton = nullptr;
    QPushButton *m_recheckButton = nullptr;
    QCheckBox *m_desktopShortcutCheck = nullptr;
    QCheckBox *m_menuShortcutCheck = nullptr;
    QTextEdit *m_logOutput = nullptr;
    QProgressBar *m_progressBar = nullptr;
};

#endif // INSTALLERWINDOW_H
