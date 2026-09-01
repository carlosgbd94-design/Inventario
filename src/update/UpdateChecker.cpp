#include "update/UpdateChecker.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include "version.h"

namespace update {

namespace {
QString stripLeadingV(const QString& version) {
    return version.startsWith('v', Qt::CaseInsensitive) ? version.mid(1) : version;
}
} // namespace

int compareVersions(const QString& a, const QString& b) {
    const QStringList partsA = stripLeadingV(a).split('.');
    const QStringList partsB = stripLeadingV(b).split('.');

    for (int i = 0; i < qMax(partsA.size(), partsB.size()); ++i) {
        const int valueA = i < partsA.size() ? partsA[i].toInt() : 0;
        const int valueB = i < partsB.size() ? partsB[i].toInt() : 0;
        if (valueA != valueB) {
            return valueA - valueB;
        }
    }
    return 0;
}

UpdateChecker::UpdateChecker(QObject* parent) : QObject(parent) {
    m_network = new QNetworkAccessManager(this);
}

void UpdateChecker::checkForUpdates() {
    QNetworkRequest request(QUrl(QString("https://api.github.com/repos/%1/releases/latest").arg(APP_GITHUB_REPO)));
    request.setHeader(QNetworkRequest::UserAgentHeader, QString(APP_NAME "/") + APP_VERSION_STRING);
    request.setRawHeader("Accept", "application/vnd.github+json");

    QNetworkReply* reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onReplyFinished(reply); });
}

void UpdateChecker::onReplyFinished(QNetworkReply* reply) {
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit checkFailed(reply->errorString());
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (!doc.isObject()) {
        emit checkFailed("Respuesta inesperada del servidor.");
        return;
    }

    const QJsonObject root = doc.object();
    const QString tagName = root.value("tag_name").toString();
    if (tagName.isEmpty()) {
        emit checkFailed("No se encontro ninguna version publicada.");
        return;
    }

    const QString latestVersion = stripLeadingV(tagName);
    if (compareVersions(latestVersion, APP_VERSION_STRING) <= 0) {
        emit upToDate();
        return;
    }

    UpdateInfo info;
    info.version = latestVersion;
    info.releaseNotes = root.value("body").toString();

    for (const QJsonValue& assetValue : root.value("assets").toArray()) {
        const QJsonObject asset = assetValue.toObject();
        const QString name = asset.value("name").toString();
        if (name.endsWith(".exe", Qt::CaseInsensitive)) {
            info.assetName = name;
            info.downloadUrl = asset.value("browser_download_url").toString();
            break;
        }
    }

    if (info.downloadUrl.isEmpty()) {
        emit checkFailed("La nueva version no tiene un instalador adjunto.");
        return;
    }

    emit updateAvailable(info);
}

} // namespace update
