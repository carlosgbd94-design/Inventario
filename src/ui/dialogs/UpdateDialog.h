#pragma once

#include <QDialog>

#include "update/UpdateChecker.h"

class QLabel;
class QProgressBar;
class QPushButton;
class QTextEdit;

namespace update {
class UpdateDownloader;
}

namespace ui {

// Ventana de actualizacion: muestra la version nueva y sus notas, deja
// descargar con una barra de progreso, y al terminar ofrece instalar
// (lo que cierra la app para que el instalador pueda reemplazarla) o
// posponer. Nada pasa sin que el usuario lo confirme aqui.
class UpdateDialog : public QDialog {
    Q_OBJECT

public:
    explicit UpdateDialog(const update::UpdateInfo& info, QWidget* parent = nullptr);
    ~UpdateDialog() override;

private:
    void onDownloadClicked();
    void onInstallClicked();

    update::UpdateInfo m_info;
    update::UpdateDownloader* m_downloader = nullptr;
    QString m_downloadedPath;

    QLabel* m_titleLabel = nullptr;
    QTextEdit* m_notesEdit = nullptr;
    QProgressBar* m_progressBar = nullptr;
    QPushButton* m_actionButton = nullptr;
    QPushButton* m_laterButton = nullptr;
};

} // namespace ui
