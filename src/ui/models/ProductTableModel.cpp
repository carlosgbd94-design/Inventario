#include "ui/models/ProductTableModel.h"

#include <QColor>
#include <QLocale>
#include <cmath>

namespace ui {

ProductTableModel::ProductTableModel(QObject* parent) : QAbstractTableModel(parent) {}

void ProductTableModel::setProducts(QVector<data::Product> products) {
    beginResetModel();
    m_products = std::move(products);
    endResetModel();
}

const data::Product& ProductTableModel::productAt(int row) const {
    return m_products.at(row);
}

int ProductTableModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : m_products.size();
}

int ProductTableModel::columnCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant ProductTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_products.size()) {
        return {};
    }

    const data::Product& product = m_products.at(index.row());
    static const QLocale locale(QLocale::Spanish, QLocale::Mexico);

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case ColName: return product.name;
            case ColVariant: return product.variant;
            case ColUnit: return product.unit;
            case ColQty: return locale.toString(product.currentQty, 'f', product.currentQty == std::floor(product.currentQty) ? 0 : 2);
            case ColCost: return locale.toCurrencyString(product.unitCost, "$");
            case ColValue: return locale.toCurrencyString(product.currentQty * product.unitCost, "$");
        }
    }

    if (role == Qt::TextAlignmentRole) {
        if (index.column() == ColQty || index.column() == ColCost || index.column() == ColValue) {
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        }
        return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
    }

    if (role == Qt::BackgroundRole) {
        if (product.minStock > 0.0 && product.currentQty <= product.minStock) {
            return QColor("#2E2718");
        }
    }

    if (role == Qt::ForegroundRole) {
        if (index.column() == ColQty && product.minStock > 0.0 && product.currentQty <= product.minStock) {
            return QColor("#E4B074");
        }
    }

    return {};
}

QVariant ProductTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QAbstractTableModel::headerData(section, orientation, role);
    }

    switch (section) {
        case ColName: return "Producto";
        case ColVariant: return "Variante";
        case ColUnit: return "Unidad";
        case ColQty: return "Cantidad";
        case ColCost: return "Costo unitario";
        case ColValue: return "Valor";
    }
    return {};
}

} // namespace ui
