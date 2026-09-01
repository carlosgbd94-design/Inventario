#include "ui/MainWindow.h"

#include <QAction>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QStackedWidget>
#include <QSvgWidget>
#include <QTimer>
#include <QVBoxLayout>

#include "app/Animations.h"
#include "ui/dialogs/UpdateDialog.h"
#include "ui/views/CutoffView.h"
#include "ui/views/DashboardView.h"
#include "ui/views/InventoryView.h"
#include "ui/widgets/NavRail.h"
#include "version.h"

namespace ui {

MainWindow::MainWindow(QSqlDatabase& db, QWidget* parent)
    : QMainWindow(parent),
      m_categoryRepo(db),
      m_productRepo(db),
      m_movementRepo(db),
      m_cutoffRepo(db),
      m_inventoryEngine(db, m_productRepo, m_movementRepo),
      m_cutoffEngine(db, m_productRepo, m_cutoffRepo),
      m_reportEngine(m_productRepo, m_categoryRepo, m_cutoffRepo) {
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

    auto* logo = new QSvgWidget(":/logo.svg", card);
    logo->setFixedSize(88, 88);
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

    m_inventoryView = new InventoryView(m_categoryRepo, m_productRepo, m_movementRepo, m_inventoryEngine,
                                         m_reportEngine, m_contentStack);
    m_contentStack->addWidget(m_inventoryView);

    m_cutoffView = new CutoffView(m_productRepo, m_categoryRepo, m_cutoffRepo, m_cutoffEngine, m_reportEngine,
                                   m_contentStack);
    m_contentStack->addWidget(m_cutoffView);

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

    QAction* aboutAction = helpMenu->addAction(QString("Acerca de %1").arg(APP_NAME));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
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
    QMessageBox::about(this, QString("Acerca de %1").arg(APP_NAME),
                        QString("<h3>%1</h3>"
                                "<p>Version %2</p>"
                                "<p>Inventario de uniformes y papeleria.</p>")
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
