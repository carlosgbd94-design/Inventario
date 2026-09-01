#include "ui/MainWindow.h"

#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QStackedWidget>
#include <QSvgWidget>
#include <QVBoxLayout>

#include "app/Animations.h"
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
      m_cutoffEngine(db, m_productRepo, m_cutoffRepo) {
    setObjectName("AppRoot");
    setWindowTitle(APP_NAME);
    resize(1180, 760);

    m_pageStack = new QStackedWidget(this);
    m_pageStack->addWidget(buildIntroPage());
    m_pageStack->addWidget(buildShellPage());
    setCentralWidget(m_pageStack);

    refreshNavCategories();
    refreshDashboard();
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

    m_inventoryView = new InventoryView(m_categoryRepo, m_productRepo, m_movementRepo, m_inventoryEngine, m_contentStack);
    m_contentStack->addWidget(m_inventoryView);

    connect(m_navRail, &NavRail::dashboardSelected, this, [this]() {
        refreshDashboard();
        app::animatedSetCurrentIndex(m_contentStack, 0);
    });
    connect(m_navRail, &NavRail::categorySelected, this, [this](const data::Category& category) {
        m_inventoryView->setCategory(category);
        app::animatedSetCurrentIndex(m_contentStack, 1);
    });
    connect(m_navRail, &NavRail::addCategoryRequested, this, &MainWindow::onAddCategory);
    connect(m_inventoryView, &InventoryView::inventoryChanged, this, &MainWindow::refreshDashboard);

    return root;
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
