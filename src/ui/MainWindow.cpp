#include "ui/MainWindow.h"

#include <QAction>
#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPixmap>
#include <QProcess>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QSqlError>
#include <QSqlQuery>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QTimer>
#include <QVBoxLayout>

#include "app/Animations.h"
#include "data/Database.h"
#include "ui/dialogs/UpdateDialog.h"
#include "ui/views/CutoffView.h"
#include "ui/views/DashboardView.h"
#include "ui/views/InventoryView.h"
#include "ui/views/SupplierView.h"
#include "ui/widgets/NavRail.h"
#include "version.h"

namespace ui {

namespace {
QString attachmentsPath() {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/attachments";
}

// QDir no trae una copia recursiva; recorre el arbol de archivos y
// reconstruye la misma estructura de carpetas en el destino.
void copyDirRecursive(const QString& srcPath, const QString& destPath) {
    QDir().mkpath(destPath);
    QDirIterator it(srcPath, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString filePath = it.next();
        const QString relative = QDir(srcPath).relativeFilePath(filePath);
        const QString destFile = destPath + "/" + relative;
        QDir().mkpath(QFileInfo(destFile).absolutePath());
        QFile::copy(filePath, destFile);
    }
}
} // namespace

MainWindow::MainWindow(QSqlDatabase& db, QWidget* parent)
    : QMainWindow(parent),
      m_db(db),
      m_categoryRepo(db),
      m_productRepo(db),
      m_movementRepo(db),
      m_cutoffRepo(db),
      m_supplierRepo(db),
      m_inventoryEngine(db, m_productRepo, m_movementRepo),
      m_cutoffEngine(db, m_productRepo, m_cutoffRepo),
      m_reportEngine(m_productRepo, m_categoryRepo, m_cutoffRepo, m_movementRepo) {
    setObjectName("AppRoot");
    setWindowTitle(APP_NAME);
    resize(1180, 760);

    m_pageStack = new QStackedWidget(this);
    m_pageStack->addWidget(buildIntroPage());
    m_pageStack->addWidget(buildShellPage());
    setCentralWidget(m_pageStack);

    buildMenuBar();
    refreshNavCategories();
    refreshDashboard();

    // Revision silenciosa al abrir: si no hay actualizacion o falla la
    // consulta (sin internet, etc.) no molestamos a nadie con avisos.
    QTimer::singleShot(1500, this, [this]() { checkForUpdates(false); });
}

QWidget* MainWindow::buildIntroPage() {
    auto* root = new QWidget(this);
    root->setObjectName("AppRoot");
    root->setAttribute(Qt::WA_StyledBackground, true);

    auto* rootLayout = new QVBoxLayout(root);
    rootLayout->setAlignment(Qt::AlignCenter);

    auto* card = new QWidget(root);
    card->setObjectName("SurfaceCard");
    card->setAttribute(Qt::WA_StyledBackground, true);
    card->setFixedWidth(360);

    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(40, 40, 40, 36);
    cardLayout->setSpacing(14);
    cardLayout->setAlignment(Qt::AlignHCenter);

    auto* logo = new QLabel(card);
    logo->setPixmap(QPixmap(":/logo.png").scaled(88, 88, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    logo->setFixedSize(88, 88);
    logo->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(logo, 0, Qt::AlignHCenter);

    auto* title = new QLabel(APP_NAME, card);
    title->setObjectName("TitleLabel");
    title->setAlignment(Qt::AlignHCenter);
    cardLayout->addWidget(title);

    auto* subtitle = new QLabel("Inventario de uniformes y papeleria", card);
    subtitle->setObjectName("SubtitleLabel");
    subtitle->setAlignment(Qt::AlignHCenter);
    cardLayout->addWidget(subtitle);

    cardLayout->addSpacing(10);

    auto* startButton = new QPushButton("Comenzar", card);
    startButton->setObjectName("PrimaryButton");
    startButton->setCursor(Qt::PointingHandCursor);
    cardLayout->addWidget(startButton, 0, Qt::AlignHCenter);
    connect(startButton, &QPushButton::clicked, this, [this]() { app::animatedSetCurrentIndex(m_pageStack, 1); });

    cardLayout->addSpacing(6);

    auto* version = new QLabel(QString("v%1").arg(APP_VERSION_STRING), card);
    version->setObjectName("VersionLabel");
    version->setAlignment(Qt::AlignHCenter);
    cardLayout->addWidget(version);

    rootLayout->addWidget(card);

    return root;
}

QWidget* MainWindow::buildShellPage() {
    auto* root = new QWidget(this);
    root->setObjectName("AppRoot");
    root->setAttribute(Qt::WA_StyledBackground, true);

    auto* layout = new QHBoxLayout(root);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_navRail = new NavRail(root);
    layout->addWidget(m_navRail);

    m_contentStack = new QStackedWidget(root);
    layout->addWidget(m_contentStack, 1);

    m_dashboard = new DashboardView(m_contentStack);
    m_contentStack->addWidget(m_dashboard);

    m_inventoryView = new InventoryView(m_categoryRepo, m_productRepo, m_movementRepo, m_supplierRepo,
                                         m_inventoryEngine, m_reportEngine, m_contentStack);
    m_contentStack->addWidget(m_inventoryView);

    m_cutoffView = new CutoffView(m_productRepo, m_categoryRepo, m_cutoffRepo, m_cutoffEngine, m_reportEngine,
                                   m_contentStack);
    m_contentStack->addWidget(m_cutoffView);

    m_supplierView = new SupplierView(m_supplierRepo, m_contentStack);
    m_contentStack->addWidget(m_supplierView);

    connect(m_navRail, &NavRail::dashboardSelected, this, [this]() {
        refreshDashboard();
        app::animatedSetCurrentIndex(m_contentStack, 0);
    });
    connect(m_navRail, &NavRail::categorySelected, this, [this](const data::Category& category) {
        m_inventoryView->setCategory(category);
        app::animatedSetCurrentIndex(m_contentStack, 1);
    });
    connect(m_navRail, &NavRail::cutoffsSelected, this, [this]() {
        m_cutoffView->reload();
        app::animatedSetCurrentIndex(m_contentStack, 2);
    });
    connect(m_navRail, &NavRail::suppliersSelected, this, [this]() {
        m_supplierView->reload();
        app::animatedSetCurrentIndex(m_contentStack, 3);
    });
    connect(m_navRail, &NavRail::addCategoryRequested, this, &MainWindow::onAddCategory);
    connect(m_inventoryView, &InventoryView::inventoryChanged, this, &MainWindow::refreshDashboard);
    connect(m_cutoffView, &CutoffView::cutoffsChanged, this, &MainWindow::refreshDashboard);

    return root;
}

void MainWindow::buildMenuBar() {
    QMenu* helpMenu = menuBar()->addMenu("Ayuda");

    QAction* checkUpdatesAction = helpMenu->addAction("Buscar actualizaciones...");
    connect(checkUpdatesAction, &QAction::triggered, this, [this]() { checkForUpdates(true); });

    helpMenu->addSeparator();

    QAction* backupAction = helpMenu->addAction("Respaldar base de datos...");
    connect(backupAction, &QAction::triggered, this, &MainWindow::onBackupDatabase);

    QAction* restoreAction = helpMenu->addAction("Restaurar desde respaldo...");
    connect(restoreAction, &QAction::triggered, this, &MainWindow::onRestoreDatabase);

    helpMenu->addSeparator();

    QAction* aboutAction = helpMenu->addAction(QString("Acerca de %1").arg(APP_NAME));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
}

void MainWindow::onBackupDatabase() {
    const QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString destDir = QFileDialog::getExistingDirectory(this, "Respaldar base de datos - elige una carpeta destino", defaultDir);
    if (destDir.isEmpty()) {
        return;
    }

    const QString stamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HHmm");
    const QString backupFolder = destDir + "/Inventario_respaldo_" + stamp;
    QDir().mkpath(backupFolder);

    // VACUUM INTO arma una copia consistente de la base sin tener que
    // cerrar la conexion activa ni pausar al usuario.
    QSqlQuery query(m_db);
    query.prepare("VACUUM INTO ?");
    query.addBindValue(backupFolder + "/inventario.db");
    if (!query.exec()) {
        QMessageBox::warning(this, "Respaldar base de datos",
                              "No se pudo respaldar la base de datos: " + query.lastError().text());
        return;
    }

    if (QDir(attachmentsPath()).exists()) {
        copyDirRecursive(attachmentsPath(), backupFolder + "/attachments");
    }

    QMessageBox::information(this, "Respaldar base de datos", "Respaldo guardado en:\n" + backupFolder);
}

void MainWindow::onRestoreDatabase() {
    const QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString srcFolder =
        QFileDialog::getExistingDirectory(this, "Restaurar desde respaldo - elige la carpeta del respaldo", defaultDir);
    if (srcFolder.isEmpty()) {
        return;
    }

    const QString srcDb = srcFolder + "/inventario.db";
    if (!QFile::exists(srcDb)) {
        QMessageBox::warning(this, "Restaurar base de datos",
                              "La carpeta elegida no contiene un respaldo valido (no se encontro inventario.db).");
        return;
    }

    const auto answer = QMessageBox::warning(
        this, "Restaurar base de datos",
        "Esto reemplazara TODO el inventario actual con el contenido del respaldo elegido. "
        "Esta accion no se puede deshacer.\n\n¿Continuar?",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }

    const QString livePath = data::Database::defaultDatabasePath();
    const QString tempPath = livePath + ".restoring";
    QFile::remove(tempPath);
    if (!QFile::copy(srcDb, tempPath)) {
        QMessageBox::critical(this, "Restaurar base de datos", "No se pudo leer el archivo de respaldo.");
        return;
    }

    m_db.close();
    QFile::remove(livePath);
    QFile::rename(tempPath, livePath);

    const QString attachmentsSrc = srcFolder + "/attachments";
    if (QDir(attachmentsSrc).exists()) {
        QDir(attachmentsPath()).removeRecursively();
        copyDirRecursive(attachmentsSrc, attachmentsPath());
    }

    QMessageBox::information(this, "Restaurar base de datos",
                              "Listo. La aplicacion se va a reiniciar para cargar el respaldo.");
    QProcess::startDetached(QApplication::applicationFilePath(), {});
    QApplication::quit();
}

void MainWindow::refreshNavCategories() {
    m_navRail->setCategories(m_categoryRepo.all());
}

void MainWindow::refreshDashboard() {
    const auto products = m_productRepo.all(true);

    DashboardStats stats;
    stats.totalProducts = products.size();
    stats.categoryCount = m_categoryRepo.all().size();
    stats.lowStockCount = m_inventoryEngine.reorderSuggestions().size();
    for (const data::Product& product : products) {
        stats.totalValue += product.currentQty * product.unitCost;
    }

    m_dashboard->setStats(stats);
}

void MainWindow::onAddCategory() {
    bool ok = false;
    const QString name = QInputDialog::getText(this, "Nueva categoria", "Nombre de la categoria:",
                                                 QLineEdit::Normal, {}, &ok)
                              .trimmed();
    if (!ok || name.isEmpty()) {
        return;
    }

    data::Category category;
    category.name = name;
    category.color = "#7C9CBF";
    m_categoryRepo.insert(category);

    refreshNavCategories();
    refreshDashboard();
}

void MainWindow::checkForUpdates(bool manual) {
    if (manual && m_manualUpdateCheckInFlight) {
        return;
    }
    m_manualUpdateCheckInFlight = manual;

    auto* checker = new update::UpdateChecker(this);

    connect(checker, &update::UpdateChecker::updateAvailable, this, [this, checker](const update::UpdateInfo& info) {
        m_manualUpdateCheckInFlight = false;
        checker->deleteLater();

        auto* dialog = new UpdateDialog(info, this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->show();
    });

    connect(checker, &update::UpdateChecker::upToDate, this, [this, checker, manual]() {
        m_manualUpdateCheckInFlight = false;
        checker->deleteLater();
        if (manual) {
            QMessageBox::information(this, "Buscar actualizaciones", "Ya tienes la version mas reciente.");
        }
    });

    connect(checker, &update::UpdateChecker::checkFailed, this, [this, checker, manual](const QString& error) {
        m_manualUpdateCheckInFlight = false;
        checker->deleteLater();
        if (manual) {
            QMessageBox::warning(this, "Buscar actualizaciones", "No se pudo comprobar si hay actualizaciones: " + error);
        }
    });

    checker->checkForUpdates();
}

void MainWindow::onAbout() {
    QMessageBox::about(
        this, QString("Acerca de %1").arg(APP_NAME),
        QString("<h2>%1</h2>"
                "<p style='color:#A9AFBC'>Version %2</p>"
                "<p>Sistema de control de inventario para uniformes y papeleria: altas y bajas de "
                "producto, movimientos con bitacora y adjuntos, catalogo de proveedores, cortes "
                "mensuales congelados con comparativo, exportacion a PDF, y actualizaciones "
                "automaticas.</p>"
                "<p><b>Creado y programado por Carlos Becerra.</b><br>"
                "Desarrollado con C++20 y Qt 6 sobre una base de datos SQLite 100% local: "
                "nada de tu inventario sale de esta computadora salvo cuando tu decides "
                "exportarlo a PDF.</p>")
            .arg(APP_NAME, APP_VERSION_STRING));
}

void MainWindow::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);

    if (m_introPlayed) {
        return;
    }
    m_introPlayed = true;

    auto* effect = new QGraphicsOpacityEffect(centralWidget());
    centralWidget()->setGraphicsEffect(effect);

    auto* fadeIn = new QPropertyAnimation(effect, "opacity", this);
    fadeIn->setDuration(220);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->setEasingCurve(QEasingCurve::OutCubic);

    QWidget* central = centralWidget();
    connect(fadeIn, &QPropertyAnimation::finished, central, [central]() { central->setGraphicsEffect(nullptr); });

    fadeIn->start(QAbstractAnimation::DeleteWhenStopped);
}

} // namespace ui
