#include "ui/models/ProductFilterProxyModel.h"

#include "ui/models/ProductTableModel.h"

namespace ui {

ProductFilterProxyModel::ProductFilterProxyModel(QObject* parent) : QSortFilterProxyModel(parent) {}

void ProductFilterProxyModel::setLowStockOnly(bool enabled) {
    m_lowStockOnly = enabled;
    invalidateRowsFilter();
}

bool ProductFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
    if (!QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent)) {
        return false;
    }
    if (!m_lowStockOnly) {
        return true;
    }

    auto* model = qobject_cast<ProductTableModel*>(sourceModel());
    if (!model) {
        return true;
    }
    const data::Product& product = model->productAt(sourceRow);
    return product.minStock > 0.0 && product.currentQty <= product.minStock;
}

} // namespace ui
