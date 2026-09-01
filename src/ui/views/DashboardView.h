#pragma once

#include <QWidget>

namespace ui {

class KpiCard;

struct DashboardStats {
    int totalProducts = 0;
    double totalValue = 0.0;
    int lowStockCount = 0;
    int categoryCount = 0;
};

class DashboardView : public QWidget {
    Q_OBJECT

public:
    explicit DashboardView(QWidget* parent = nullptr);

    void setStats(const DashboardStats& stats);

private:
    KpiCard* m_valueCard = nullptr;
    KpiCard* m_lowStockCard = nullptr;
    KpiCard* m_productsCard = nullptr;
    KpiCard* m_categoriesCard = nullptr;
};

} // namespace ui
