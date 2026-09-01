#include "ui/dialogs/UpdateDialog.h"

#include <QCoreApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

#include "update/UpdateDownloader.h"
#include "version.h"

namespace ui {

UpdateDialog::UpdateDialog(const update::UpdateInfo& info, QWidget* parent) : QDialog(parent), m_info(info) {
    setWindowTitle("Actualizacion disponible");
    setModal(true);
    setMinimumSize(420, 320);

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setSpacing(14);

    m_titleLabel = new QLabel(QString("Hay una nueva version disponible: v%1").arg(info.version), this);
    m_titleLabel->setObjectName("TitleLabel");
    m_titleLabel->setWordWrap(true);
    rootLayout->addWidget(m_titleLabel);

    auto* currentLabel = new QLabel(QString("Version instalada: v%1").arg(APP_VERSION_STRING), this);
    currentLabel->setObjectName("SubtitleLabel");
    rootLayout->addWidget(currentLabel);

    m_notesEdit = new QTextEdit(this);
    m_notesEdit->setReadOnly(true);
    m_notesEdit->setPlainText(info.releaseNotes.isEmpty() ? "Sin notas de esta version." : info.releaseNotes);
    rootLayout->addWidget(m_notesEdit, 1);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setVisible(false);
    rootLayout->addWidget(m_progressBar);

    auto* buttonsLayout = new QHBoxLayout();
    m_laterButton = new QPushButton("Mas tarde", this);
    m_laterButton->setCursor(Qt::PointingHandCursor);
    buttonsLayout->addWidget(m_laterButton);
    buttonsLayout->addStretch(1);

    m_actionButton = new QPushButton("Descargar", this);
    m_actionButton->setObjectName("PrimaryButton");
    m_actionButton->setCursor(Qt::PointingHandCursor);
    buttonsLayout->addWidget(m_actionButton);
    rootLayout->addLayout(buttonsLayout);

    connect(m_laterButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_actionButton, &QPushButton::clicked, this, &UpdateDialog::onDownloadClicked);
}

UpdateDialog::~UpdateDialog() = default;

void UpdateDialog::onDownloadClicked() {
    m_actionButton->setEnabled(false);
    m_actionButton->setText("Descargando...");
    m_progressBar->setVisible(true);
    m_progressBar->setRange(0, 0); // indeterminado hasta que sepamos el total

    m_downloader = new update::UpdateDownloader(this);

    connect(m_downloader, &update::UpdateDownloader::progress, this, [this](qint64 received, qint64 total) {
        if (total > 0) {
            m_progressBar->setRange(0, 100);
            m_progressBar->setValue(static_cast<int>((received * 100) / total));
        }
    });

    connect(m_downloader, &update::UpdateDownloader::finished, this, [this](const QString& localPath) {
        m_downloadedPath = localPath;
        m_progressBar->setRange(0, 100);
        m_progressBar->setValue(100);
        m_actionButton->setText("Instalar ahora");
        m_actionButton->setEnabled(true);
        disconnect(m_actionButton, &QPushButton::clicked, this, &UpdateDialog::onDownloadClicked);
        connect(m_actionButton, &QPushButton::clicked, this, &UpdateDialog::onInstallClicked);
    });

    connect(m_downloader, &update::UpdateDownloader::failed, this, [this](const QString& error) {
        m_progressBar->setVisible(false);
        m_actionButton->setText("Descargar");
        m_actionButton->setEnabled(true);
        QMessageBox::warning(this, "Actualizacion", "No se pudo descargar la actualizacion: " + error);
    });

    m_downloader->download(m_info.downloadUrl, m_info.assetName);
}

void UpdateDialog::onInstallClicked() {
    if (!QProcess::startDetached(m_downloadedPath, {})) {
        QMessageBox::warning(this, "Actualizacion", "No se pudo iniciar el instalador descargado.");
        return;
    }
    QCoreApplication::quit();
}

} // namespace ui
