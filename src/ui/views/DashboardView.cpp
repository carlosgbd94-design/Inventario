#include "ui/views/DashboardView.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QVBoxLayout>

#include "ui/widgets/KpiCard.h"

namespace ui {

DashboardView::DashboardView(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(32, 28, 32, 28);
    layout->setSpacing(20);

    auto* title = new QLabel("Panel general", this);
    title->setObjectName("TitleLabel");
    layout->addWidget(title);

    auto* cardsLayout = new QHBoxLayout();
    cardsLayout->setSpacing(16);

    m_valueCard = new KpiCard("Valor total del inventario", this);
    m_valueCard->setFormatter([](double v) {
        return QLocale(QLocale::Spanish, QLocale::Mexico).toCurrencyString(v, "$");
    });

    m_lowStockCard = new KpiCard("Productos bajo minimo", this);
    m_productsCard = new KpiCard("Productos activos", this);
    m_categoriesCard = new KpiCard("Categorias", this);

    cardsLayout->addWidget(m_valueCard);
    cardsLayout->addWidget(m_lowStockCard);
    cardsLayout->addWidget(m_productsCard);
    cardsLayout->addWidget(m_categoriesCard);

    layout->addLayout(cardsLayout);
    layout->addStretch(1);
}

void DashboardView::setStats(const DashboardStats& stats) {
    m_valueCard->setValue(stats.totalValue);
    m_lowStockCard->setValue(stats.lowStockCount);
    m_productsCard->setValue(stats.totalProducts);
    m_categoriesCard->setValue(stats.categoryCount);
}

} // namespace ui
