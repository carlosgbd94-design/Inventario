#pragma once

#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;
class QFile;

namespace update {

// Descarga el instalador de una nueva version a una carpeta temporal,
// reportando progreso para poder mostrar una barra animada en la UI.
class UpdateDownloader : public QObject {
    Q_OBJECT

public:
    explicit UpdateDownloader(QObject* parent = nullptr);

    void download(const QString& url, const QString& suggestedFileName);
    void cancel();

signals:
    void progress(qint64 bytesReceived, qint64 bytesTotal);
    void finished(const QString& localFilePath);
    void failed(const QString& error);

private:
    QNetworkAccessManager* m_network;
    QNetworkReply* m_reply = nullptr;
    QFile* m_file = nullptr;
    QString m_localPath;
};

} // namespace update
