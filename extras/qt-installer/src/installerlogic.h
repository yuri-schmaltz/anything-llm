#ifndef INSTALLERLOGIC_H
#define INSTALLERLOGIC_H

#include <QObject>
#include <QString>
#include <QMetaType>

class InstallerLogic : public QObject {
    Q_OBJECT
public:
    explicit InstallerLogic(QObject *parent = nullptr);

    enum class InstallAction {
        FreshInstall,
        UpdateExisting,
        RepairExisting
    };
    Q_ENUM(InstallAction)

    struct InstallationStatus {
        bool installed = false;
        bool updateAvailable = false;
        bool repairAvailable = false;
        QString installedVersion;
        QString availableVersion;
        QString installPath;
        InstallAction recommendedAction = InstallAction::FreshInstall;
    };

    struct InstallResult {
        bool success = false;
        QString message;
    };

    void startDetection();
    void startInstallation(const QString &targetPath,
                           InstallAction action,
                           bool createDesktopShortcut,
                           bool createMenuShortcut);

    QString defaultInstallPath() const;
    QString availableVersion() const;

signals:
    void detectionFinished(const InstallerLogic::InstallationStatus &status);
    void installationProgress(const QString &message);
    void installationStep(int progressValue);
    void installationFinished(const InstallerLogic::InstallResult &result);

private:
    InstallationStatus detectInstallation() const;
    InstallResult performInstallation(const QString &targetPath,
                                      InstallAction action,
                                      bool createDesktopShortcut,
                                      bool createMenuShortcut);

    QString installerStateFilePath() const;
    bool saveInstallerState(const QString &path) const;
    QString payloadDirectory() const;
    bool ensureTargetDirectory(const QString &path, QString &error, InstallAction action) const;
    bool copyPayload(const QString &targetPath, QString &error);
    bool copyDirectoryRecursively(const QString &source, const QString &destination, QString &error);
    qint64 countPayloadFiles(const QString &source) const;
    int compareVersions(const QString &left, const QString &right) const;
    QString executablePathForShortcuts(const QString &installDir) const;
    bool createShortcuts(const QString &targetPath, bool desktop, bool menu, QString &error) const;
    bool createDesktopShortcut(const QString &targetPath, const QString &executable, QString &error) const;
    bool createMenuShortcut(const QString &targetPath, const QString &executable, QString &error) const;

    QString m_availableVersion;
    qint64 m_totalFiles = 0;
    qint64 m_copiedFiles = 0;
};

Q_DECLARE_METATYPE(InstallerLogic::InstallationStatus)
Q_DECLARE_METATYPE(InstallerLogic::InstallResult)

#endif // INSTALLERLOGIC_H
