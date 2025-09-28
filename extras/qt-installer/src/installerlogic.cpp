#include "installerlogic.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLatin1String>
#include <QtGlobal>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QtConcurrent>

#include <algorithm>

namespace {
QString sanitizePath(QString path) {
    QDir dir(path);
    return dir.absolutePath();
}
}

InstallerLogic::InstallerLogic(QObject *parent)
    : QObject(parent),
      m_availableVersion(QStringLiteral(APP_VERSION)) {
    qRegisterMetaType<InstallerLogic::InstallationStatus>("InstallerLogic::InstallationStatus");
    qRegisterMetaType<InstallerLogic::InstallResult>("InstallerLogic::InstallResult");
}

void InstallerLogic::startDetection() {
    QtConcurrent::run([this]() {
        InstallationStatus status = detectInstallation();
        emit detectionFinished(status);
    });
}

void InstallerLogic::startInstallation(const QString &targetPath,
                                       InstallAction action,
                                       bool createDesktopShortcut,
                                       bool createMenuShortcut) {
    const QString sanitizedPath = sanitizePath(targetPath);
    QtConcurrent::run([this, sanitizedPath, action, createDesktopShortcut, createMenuShortcut]() {
        InstallResult result = performInstallation(sanitizedPath, action, createDesktopShortcut, createMenuShortcut);
        emit installationFinished(result);
    });
}

QString InstallerLogic::defaultInstallPath() const {
#ifdef Q_OS_WIN
    QString base = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    if (base.isEmpty()) {
        base = QDir::home().filePath("AppData/Local/Programs");
    }
    return QDir(base).filePath("AnythingLLM");
#elif defined(Q_OS_MACOS)
    return QStringLiteral("/Applications/AnythingLLM");
#else
    QString base = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    if (base.isEmpty()) {
        base = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/Aplicativos";
    }
    return QDir(base).filePath("AnythingLLM");
#endif
}

QString InstallerLogic::availableVersion() const {
    return m_availableVersion;
}

InstallerLogic::InstallationStatus InstallerLogic::detectInstallation() const {
    InstallationStatus status;
    status.availableVersion = m_availableVersion;
    status.installPath = defaultInstallPath();

    QFile stateFile(installerStateFilePath());
    if (stateFile.exists() && stateFile.open(QIODevice::ReadOnly)) {
        const QJsonDocument doc = QJsonDocument::fromJson(stateFile.readAll());
        const QJsonObject obj = doc.object();
        status.installPath = obj.value(QStringLiteral("path")).toString(status.installPath);
        status.installedVersion = obj.value(QStringLiteral("version")).toString();
        stateFile.close();
    }

    if (!status.installPath.isEmpty()) {
        status.installPath = sanitizePath(status.installPath);
    }

    if (!status.installedVersion.isEmpty()) {
        status.installed = true;
        QDir installDir(status.installPath);
        const bool pathExists = installDir.exists();
        status.repairAvailable = true;
        if (!pathExists) {
            status.recommendedAction = InstallAction::RepairExisting;
        } else {
            const int cmp = compareVersions(status.installedVersion, status.availableVersion);
            status.updateAvailable = cmp < 0;
            status.recommendedAction = status.updateAvailable ? InstallAction::UpdateExisting : InstallAction::RepairExisting;
        }
    } else {
        status.installed = false;
        status.recommendedAction = InstallAction::FreshInstall;
    }

    if (status.installPath.isEmpty()) {
        status.installPath = defaultInstallPath();
    }

    return status;
}

InstallerLogic::InstallResult InstallerLogic::performInstallation(const QString &targetPath,
                                                                  InstallAction action,
                                                                  bool createDesktopShortcut,
                                                                  bool createMenuShortcut) {
    InstallResult result;
    QString error;

    emit installationProgress(tr("Preparando instalação em %1").arg(targetPath));

    if (!ensureTargetDirectory(targetPath, error, action)) {
        result.message = error;
        return result;
    }

    if (!copyPayload(targetPath, error)) {
        result.message = error;
        return result;
    }

    if (!saveInstallerState(targetPath)) {
        result.message = tr("Não foi possível salvar o estado da instalação.");
        return result;
    }

    QString shortcutError;
    const bool shortcutsCreated = createShortcuts(targetPath, createDesktopShortcut, createMenuShortcut, shortcutError);
    if (!shortcutsCreated && !shortcutError.isEmpty()) {
        emit installationProgress(shortcutError);
    }

    emit installationStep(100);

    result.success = true;
    switch (action) {
    case InstallAction::FreshInstall:
        result.message = tr("Instalação concluída com sucesso.");
        break;
    case InstallAction::UpdateExisting:
        result.message = tr("Atualização concluída com sucesso.");
        break;
    case InstallAction::RepairExisting:
        result.message = tr("Reparo concluído com sucesso.");
        break;
    }

    if (!shortcutsCreated) {
        result.message += QLatin1Char('\n') + tr("Alguns atalhos não puderam ser criados. Consulte o log para mais detalhes.");
    }

    return result;
}

QString InstallerLogic::installerStateFilePath() const {
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (base.isEmpty()) {
        base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    }
    QDir dir(base);
    dir.mkpath(QStringLiteral("anything-llm"));
    return dir.filePath(QStringLiteral("anything-llm/installer-state.json"));
}

bool InstallerLogic::saveInstallerState(const QString &path) const {
    QFile stateFile(installerStateFilePath());
    QDir().mkpath(QFileInfo(stateFile).path());
    if (!stateFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    QJsonObject obj;
    obj.insert(QStringLiteral("path"), path);
    obj.insert(QStringLiteral("version"), m_availableVersion);
    obj.insert(QStringLiteral("modified"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

    const QJsonDocument doc(obj);
    const qint64 written = stateFile.write(doc.toJson(QJsonDocument::Indented));
    stateFile.close();
    return written > 0;
}

QString InstallerLogic::payloadDirectory() const {
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("payload"));
}

bool InstallerLogic::ensureTargetDirectory(const QString &path, QString &error, InstallAction action) const {
    QDir targetDir(path);
    if (!targetDir.exists()) {
        if (!QDir().mkpath(path)) {
            error = tr("Não foi possível criar o diretório de instalação: %1").arg(path);
            return false;
        }
    }

    if (action == InstallAction::FreshInstall) {
        return true;
    }

    // Para atualização ou reparo garantimos que o diretório seja gravável
    QFileInfo info(path);
    if (!info.isWritable()) {
        error = tr("Sem permissão de escrita em %1").arg(path);
        return false;
    }

    return true;
}

bool InstallerLogic::copyPayload(const QString &targetPath, QString &error) {
    const QString source = payloadDirectory();
    QDir sourceDir(source);
    if (!sourceDir.exists()) {
        error = tr("Pacote de instalação ausente em %1").arg(source);
        return false;
    }

    emit installationProgress(tr("Copiando arquivos da aplicação..."));

    m_totalFiles = countPayloadFiles(source);
    m_copiedFiles = 0;
    if (!copyDirectoryRecursively(source, targetPath, error)) {
        return false;
    }

    if (m_totalFiles == 0) {
        emit installationStep(100);
    }

    return true;
}

bool InstallerLogic::copyDirectoryRecursively(const QString &source, const QString &destination, QString &error) {
    QDir sourceDir(source);
    QDirIterator it(source, QDir::NoDotAndDotDot | QDir::AllEntries, QDirIterator::Subdirectories);

    while (it.hasNext()) {
        it.next();
        const QFileInfo info = it.fileInfo();
        const QString relativePath = sourceDir.relativeFilePath(info.absoluteFilePath());
        const QString target = QDir(destination).filePath(relativePath);

        if (info.isDir()) {
            if (!QDir().mkpath(target)) {
                error = tr("Não foi possível criar a pasta %1").arg(target);
                return false;
            }
        } else {
            const QString targetDir = QFileInfo(target).path();
            if (!QDir().mkpath(targetDir)) {
                error = tr("Não foi possível criar a pasta %1").arg(targetDir);
                return false;
            }
            if (QFile::exists(target)) {
                QFile::remove(target);
            }
            if (!QFile::copy(info.absoluteFilePath(), target)) {
                error = tr("Falha ao copiar %1").arg(relativePath);
                return false;
            }
            ++m_copiedFiles;
            if (m_totalFiles > 0) {
                const int percent = static_cast<int>((static_cast<double>(m_copiedFiles) / static_cast<double>(m_totalFiles)) * 100.0);
                emit installationStep(qBound(0, percent, 100));
            }
            emit installationProgress(tr("Copiado %1").arg(relativePath));
        }
    }

    return true;
}

qint64 InstallerLogic::countPayloadFiles(const QString &source) const {
    qint64 count = 0;
    QDirIterator it(source, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        ++count;
    }
    return count;
}

int InstallerLogic::compareVersions(const QString &left, const QString &right) const {
    const QStringList leftParts = left.split('.');
    const QStringList rightParts = right.split('.');
    const int maxParts = std::max(leftParts.size(), rightParts.size());

    for (int i = 0; i < maxParts; ++i) {
        const int leftValue = i < leftParts.size() ? leftParts.at(i).toInt() : 0;
        const int rightValue = i < rightParts.size() ? rightParts.at(i).toInt() : 0;
        if (leftValue < rightValue) {
            return -1;
        }
        if (leftValue > rightValue) {
            return 1;
        }
    }
    return 0;
}

QString InstallerLogic::executablePathForShortcuts(const QString &installDir) const {
#ifdef Q_OS_MACOS
    return QDir(installDir).filePath(QStringLiteral("AnythingLLM.app"));
#elif defined(Q_OS_WIN)
    return QDir(installDir).filePath(QStringLiteral("anything-llm.exe"));
#else
    return QDir(installDir).filePath(QStringLiteral("anything-llm"));
#endif
}

bool InstallerLogic::createShortcuts(const QString &targetPath, bool desktop, bool menu, QString &error) const {
    if (!desktop && !menu) {
        return true;
    }

    const QString executable = executablePathForShortcuts(targetPath);

    if (desktop && !createDesktopShortcut(targetPath, executable, error)) {
        return false;
    }

    if (menu && !createMenuShortcut(targetPath, executable, error)) {
        return false;
    }

    return true;
}

bool InstallerLogic::createDesktopShortcut(const QString &targetPath, const QString &executable, QString &error) const {
    Q_UNUSED(targetPath)
#ifdef Q_OS_WIN
    const QString desktopDir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    if (desktopDir.isEmpty()) {
        error = tr("Não foi possível localizar a pasta da área de trabalho.");
        return false;
    }
    const QString shortcutPath = QDir(desktopDir).filePath(QStringLiteral("AnythingLLM.lnk"));

    QTemporaryFile scriptFile;
    scriptFile.setAutoRemove(true);
    if (!scriptFile.open()) {
        error = tr("Não foi possível criar script temporário para o atalho.");
        return false;
    }

    const QString script = QString::fromLatin1(
        "$ErrorActionPreference='Stop';"
        "$WScriptShell=New-Object -ComObject WScript.Shell;"
        "$Shortcut=$WScriptShell.CreateShortcut('%1');"
        "$Shortcut.TargetPath='%2';"
        "$Shortcut.WorkingDirectory='%3';"
        "$Shortcut.IconLocation='%2';"
        "$Shortcut.Save();")
                               .arg(shortcutPath.replace('\\', "\\\\"))
                               .arg(executable.replace('\\', "\\\\"))
                               .arg(QFileInfo(executable).absolutePath().replace('\\', "\\\\"));

    scriptFile.write(script.toUtf8());
    scriptFile.close();

    const int exitCode = QProcess::execute(QStringLiteral("powershell"),
                                           {QStringLiteral("-NoProfile"),
                                            QStringLiteral("-ExecutionPolicy"),
                                            QStringLiteral("Bypass"),
                                            QStringLiteral("-File"),
                                            scriptFile.fileName()});
    if (exitCode != 0) {
        error = tr("Falha ao criar atalho na área de trabalho (código %1).").arg(exitCode);
        return false;
    }
    return true;
#elif defined(Q_OS_MACOS)
    const QString desktopDir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    if (desktopDir.isEmpty()) {
        error = tr("Não foi possível localizar a pasta da área de trabalho.");
        return false;
    }
    const QString linkPath = QDir(desktopDir).filePath(QStringLiteral("AnythingLLM.app"));
    QFile::remove(linkPath);
    if (!QFile::link(executable, linkPath)) {
        error = tr("Falha ao criar alias na área de trabalho.");
        return false;
    }
    return true;
#else
    const QString desktopDir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    if (desktopDir.isEmpty()) {
        error = tr("Não foi possível localizar a pasta da área de trabalho.");
        return false;
    }

    QFile desktopEntry(QDir(desktopDir).filePath(QStringLiteral("anything-llm.desktop")));
    if (!desktopEntry.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        error = tr("Falha ao criar atalho na área de trabalho.");
        return false;
    }

    const QString content = QStringLiteral(
        "[Desktop Entry]\n"
        "Type=Application\n"
        "Name=AnythingLLM\n"
        "Exec=\"%1\"\n"
        "Icon=%1\n"
        "Terminal=false\n"
        "Categories=Utility;Development;\n")
                               .arg(executable);
    desktopEntry.write(content.toUtf8());
    desktopEntry.close();
    desktopEntry.setPermissions(QFile::ExeUser | QFile::ExeGroup | QFile::ExeOther |
                                QFile::ReadUser | QFile::ReadGroup | QFile::ReadOther |
                                QFile::WriteUser);
    return true;
#endif
}

bool InstallerLogic::createMenuShortcut(const QString &targetPath, const QString &executable, QString &error) const {
    Q_UNUSED(targetPath)
#ifdef Q_OS_WIN
    QString menuDir = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    if (menuDir.isEmpty()) {
        error = tr("Não foi possível localizar o diretório do menu iniciar.");
        return false;
    }
    const QString shortcutPath = QDir(menuDir).filePath(QStringLiteral("AnythingLLM.lnk"));

    QTemporaryFile scriptFile;
    scriptFile.setAutoRemove(true);
    if (!scriptFile.open()) {
        error = tr("Não foi possível criar script temporário para o atalho.");
        return false;
    }

    const QString script = QString::fromLatin1(
        "$ErrorActionPreference='Stop';"
        "$WScriptShell=New-Object -ComObject WScript.Shell;"
        "$Shortcut=$WScriptShell.CreateShortcut('%1');"
        "$Shortcut.TargetPath='%2';"
        "$Shortcut.WorkingDirectory='%3';"
        "$Shortcut.IconLocation='%2';"
        "$Shortcut.Save();")
                               .arg(shortcutPath.replace('\\', "\\\\"))
                               .arg(executable.replace('\\', "\\\\"))
                               .arg(QFileInfo(executable).absolutePath().replace('\\', "\\\\"));

    scriptFile.write(script.toUtf8());
    scriptFile.close();

    const int exitCode = QProcess::execute(QStringLiteral("powershell"),
                                           {QStringLiteral("-NoProfile"),
                                            QStringLiteral("-ExecutionPolicy"),
                                            QStringLiteral("Bypass"),
                                            QStringLiteral("-File"),
                                            scriptFile.fileName()});
    if (exitCode != 0) {
        error = tr("Falha ao criar atalho no menu iniciar (código %1).").arg(exitCode);
        return false;
    }
    return true;
#elif defined(Q_OS_MACOS)
    Q_UNUSED(executable)
    Q_UNUSED(error)
    // No macOS criamos apenas alias na pasta Applications se necessário
    return true;
#else
    QString menuDir = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    if (menuDir.isEmpty()) {
        menuDir = QDir::home().filePath(QStringLiteral(".local/share/applications"));
    }
    QDir().mkpath(menuDir);

    QFile menuEntry(QDir(menuDir).filePath(QStringLiteral("anything-llm.desktop")));
    if (!menuEntry.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        error = tr("Falha ao criar lançador no menu de aplicativos.");
        return false;
    }

    const QString content = QStringLiteral(
        "[Desktop Entry]\n"
        "Type=Application\n"
        "Name=AnythingLLM\n"
        "Exec=\"%1\"\n"
        "Icon=%1\n"
        "Terminal=false\n"
        "Categories=Utility;Development;\n")
                               .arg(executable);
    menuEntry.write(content.toUtf8());
    menuEntry.close();
    menuEntry.setPermissions(QFile::ExeUser | QFile::ExeGroup | QFile::ExeOther |
                             QFile::ReadUser | QFile::ReadGroup | QFile::ReadOther |
                             QFile::WriteUser);
    return true;
#endif
}
