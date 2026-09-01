#include "update/UpdateDownloader.h"

#include <QDir>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>

namespace update {

UpdateDownloader::UpdateDownloader(QObject* parent) : QObject(parent) {
    m_network = new QNetworkAccessManager(this);
}

void UpdateDownloader::download(const QString& url, const QString& suggestedFileName) {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/InventarioUpdate";
    QDir().mkpath(dir);
    m_localPath = dir + "/" + suggestedFileName;

    m_file = new QFile(m_localPath, this);
    if (!m_file->open(QIODevice::WriteOnly)) {
        emit failed("No se pudo crear el archivo de descarga.");
        return;
    }

    QNetworkRequest request((QUrl(url)));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    m_reply = m_network->get(request);

    connect(m_reply, &QNetworkReply::readyRead, this, [this]() {
        if (m_file) {
            m_file->write(m_reply->readAll());
        }
    });
    connect(m_reply, &QNetworkReply::downloadProgress, this, &UpdateDownloader::progress);
    connect(m_reply, &QNetworkReply::finished, this, [this]() {
        const bool ok = m_reply->error() == QNetworkReply::NoError;
        const QString errorText = m_reply->errorString();

        if (m_file) {
            m_file->close();
        }
        m_reply->deleteLater();
        m_reply = nullptr;

        if (ok) {
            emit finished(m_localPath);
        } else {
            QFile::remove(m_localPath);
            emit failed(errorText);
        }
    });
}

void UpdateDownloader::cancel() {
    if (m_reply) {
        m_reply->abort();
    }
}

} // namespace update
