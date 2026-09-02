#pragma once

#include <QSortFilterProxyModel>

namespace ui {

// Extiende el filtro de texto normal de QSortFilterProxyModel con un
// interruptor adicional para mostrar solo los productos en o por debajo
// de su stock minimo. Ambos filtros se combinan (deben cumplirse los
// dos si el interruptor esta activo).
class ProductFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

public:
    explicit ProductFilterProxyModel(QObject* parent = nullptr);

    void setLowStockOnly(bool enabled);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
    bool m_lowStockOnly = false;
};

} // namespace ui
