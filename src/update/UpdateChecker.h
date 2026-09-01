#pragma once

#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

namespace update {

struct UpdateInfo {
    QString version; // "1.2.0", sin el prefijo "v"
    QString releaseNotes;
    QString downloadUrl; // asset .exe del release
    QString assetName;
};

// Compara dos versiones "X.Y.Z" (sin prefijo v). Regresa <0 si a<b, 0 si
// son iguales, >0 si a>b. Componentes faltantes se tratan como 0.
int compareVersions(const QString& a, const QString& b);

// Consulta el ultimo release publicado en GitHub
// (api.github.com/repos/<owner>/<repo>/releases/latest) y avisa si hay
// una version mas nueva que la actual.
class UpdateChecker : public QObject {
    Q_OBJECT

public:
    explicit UpdateChecker(QObject* parent = nullptr);

    void checkForUpdates();

signals:
    void updateAvailable(const update::UpdateInfo& info);
    void upToDate();
    void checkFailed(const QString& error);

private:
    void onReplyFinished(QNetworkReply* reply);

    QNetworkAccessManager* m_network;
};

} // namespace update
