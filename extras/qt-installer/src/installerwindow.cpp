#include "installerwindow.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QStringList>

InstallerWindow::InstallerWindow(QWidget *parent)
    : QMainWindow(parent),
      m_logic(new InstallerLogic(this)) {
    buildUi();

    connect(m_logic, &InstallerLogic::detectionFinished, this, &InstallerWindow::handleDetectionFinished);
    connect(m_logic, &InstallerLogic::installationProgress, this, &InstallerWindow::handleInstallationProgress);
    connect(m_logic, &InstallerLogic::installationStep, this, &InstallerWindow::handleInstallationStep);
    connect(m_logic, &InstallerLogic::installationFinished, this, &InstallerWindow::handleInstallationFinished);

    triggerDetection();
}

void InstallerWindow::buildUi() {
    setWindowTitle(tr("Instalador AnythingLLM"));
    setWindowIcon(QIcon(QStringLiteral(":/icons/app-icon.png")));
    resize(720, 560);

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    auto *mainLayout = new QVBoxLayout(centralWidget);

    m_statusLabel = new QLabel(tr("Verificando instalação existente..."), this);
    m_statusLabel->setWordWrap(true);
    mainLayout->addWidget(m_statusLabel);

    auto *pathLayout = new QHBoxLayout();
    auto *pathLabel = new QLabel(tr("Local de instalação:"), this);
    m_pathEdit = new QLineEdit(this);
    m_pathEdit->setPlaceholderText(tr("Selecione o diretório onde AnythingLLM será instalado"));
    auto *browseButton = new QPushButton(tr("Selecionar..."), this);
    connect(browseButton, &QPushButton::clicked, this, &InstallerWindow::browseForPath);
    pathLayout->addWidget(pathLabel);
    pathLayout->addWidget(m_pathEdit, 1);
    pathLayout->addWidget(browseButton);
    mainLayout->addLayout(pathLayout);

    auto *shortcutsGroup = new QGroupBox(tr("Atalhos"), this);
    auto *shortcutsLayout = new QVBoxLayout(shortcutsGroup);
    m_desktopShortcutCheck = new QCheckBox(tr("Criar atalho na área de trabalho"), shortcutsGroup);
    m_menuShortcutCheck = new QCheckBox(tr("Adicionar ao menu iniciar/aplicativos"), shortcutsGroup);
    m_desktopShortcutCheck->setChecked(true);
    m_menuShortcutCheck->setChecked(true);
    shortcutsLayout->addWidget(m_desktopShortcutCheck);
    shortcutsLayout->addWidget(m_menuShortcutCheck);
    shortcutsGroup->setLayout(shortcutsLayout);
    mainLayout->addWidget(shortcutsGroup);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    mainLayout->addWidget(m_progressBar);

    m_logOutput = new QTextEdit(this);
    m_logOutput->setReadOnly(true);
    m_logOutput->setPlaceholderText(tr("Mensagens de instalação aparecerão aqui."));
    mainLayout->addWidget(m_logOutput, 1);

    auto *buttonsLayout = new QHBoxLayout();
    buttonsLayout->addStretch();
    m_recheckButton = new QPushButton(tr("Verificar novamente"), this);
    connect(m_recheckButton, &QPushButton::clicked, this, &InstallerWindow::triggerDetection);
    m_installButton = new QPushButton(tr("Instalar"), this);
    connect(m_installButton, &QPushButton::clicked, this, &InstallerWindow::startInstallation);
    buttonsLayout->addWidget(m_recheckButton);
    buttonsLayout->addWidget(m_installButton);
    mainLayout->addLayout(buttonsLayout);
}

void InstallerWindow::handleDetectionFinished(const InstallerLogic::InstallationStatus &status) {
    m_currentStatus = status;
    updateUiForStatus(status);
    setUiEnabled(true);
}

void InstallerWindow::handleInstallationProgress(const QString &message) {
    appendLogMessage(message);
}

void InstallerWindow::handleInstallationStep(int value) {
    m_progressBar->setValue(value);
}

void InstallerWindow::handleInstallationFinished(const InstallerLogic::InstallResult &result) {
    m_installationInProgress = false;
    setUiEnabled(true);

    if (result.success) {
        appendLogMessage(result.message);
        QMessageBox::information(this, tr("Instalação"), result.message);
        triggerDetection();
    } else {
        appendLogMessage(result.message);
        QMessageBox::critical(this, tr("Instalação"), result.message);
    }
}

void InstallerWindow::startInstallation() {
    if (m_installationInProgress) {
        return;
    }

    const QString targetPath = m_pathEdit->text().trimmed();
    if (targetPath.isEmpty()) {
        QMessageBox::warning(this, tr("Instalação"), tr("Informe um local de instalação válido."));
        return;
    }

    InstallerLogic::InstallAction action = m_currentStatus.recommendedAction;
    if (m_currentStatus.installed && targetPath != m_currentStatus.installPath) {
        action = InstallerLogic::InstallAction::FreshInstall;
    }

    m_installationInProgress = true;
    setUiEnabled(false);
    m_progressBar->setValue(0);
    m_logOutput->clear();

    appendLogMessage(tr("Iniciando processo de instalação..."));
    m_logic->startInstallation(targetPath, action, m_desktopShortcutCheck->isChecked(), m_menuShortcutCheck->isChecked());
}

void InstallerWindow::browseForPath() {
    const QString selectedDirectory = QFileDialog::getExistingDirectory(this,
                                                                       tr("Selecione o diretório de instalação"),
                                                                       m_pathEdit->text().isEmpty() ? m_logic->defaultInstallPath() : m_pathEdit->text());
    if (!selectedDirectory.isEmpty()) {
        m_pathEdit->setText(selectedDirectory);
    }
}

void InstallerWindow::triggerDetection() {
    setUiEnabled(false);
    appendLogMessage(tr("Verificando estado da instalação..."));
    m_logic->startDetection();
}

void InstallerWindow::setUiEnabled(bool enabled) {
    const bool allowInteraction = enabled && !m_installationInProgress;
    m_pathEdit->setEnabled(allowInteraction);
    m_installButton->setEnabled(allowInteraction);
    m_recheckButton->setEnabled(enabled);
    m_desktopShortcutCheck->setEnabled(allowInteraction);
    m_menuShortcutCheck->setEnabled(allowInteraction);
}

void InstallerWindow::updateUiForStatus(const InstallerLogic::InstallationStatus &status) {
    QStringList infoLines;
    infoLines << tr("Versão disponível: %1").arg(status.availableVersion);

    if (status.installed) {
        if (!status.installedVersion.isEmpty()) {
            infoLines << tr("Versão instalada: %1").arg(status.installedVersion);
        }
        if (status.updateAvailable) {
            infoLines << tr("Uma atualização está disponível.");
        } else {
            infoLines << tr("A instalação já está atualizada.");
        }
        if (status.repairAvailable) {
            infoLines << tr("Você pode reparar a instalação atual.");
        }
    } else {
        infoLines << tr("Nenhuma instalação anterior encontrada.");
    }

    m_statusLabel->setText(infoLines.join('\n'));
    m_pathEdit->setText(status.installPath);

    switch (status.recommendedAction) {
    case InstallerLogic::InstallAction::FreshInstall:
        m_installButton->setText(tr("Instalar"));
        break;
    case InstallerLogic::InstallAction::UpdateExisting:
        m_installButton->setText(tr("Atualizar"));
        break;
    case InstallerLogic::InstallAction::RepairExisting:
        m_installButton->setText(tr("Reparar"));
        break;
    }
}

void InstallerWindow::appendLogMessage(const QString &message) {
    if (message.isEmpty()) {
        return;
    }
    m_logOutput->append(message);
}

void InstallerWindow::closeEvent(QCloseEvent *event) {
    if (m_installationInProgress) {
        const auto response = QMessageBox::question(this,
                                                    tr("Instalação em andamento"),
                                                    tr("Uma instalação está em andamento. Deseja realmente sair?"));
        if (response != QMessageBox::Yes) {
            event->ignore();
            return;
        }
    }
    QMainWindow::closeEvent(event);
}
