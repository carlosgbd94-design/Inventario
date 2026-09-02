#pragma once

#include <QAbstractTableModel>
#include <QVector>

#include "data/Models.h"

namespace ui {

class ProductTableModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column { ColSku = 0, ColName, ColVariant, ColUnit, ColQty, ColCost, ColValue, ColumnCount };

    explicit ProductTableModel(QObject* parent = nullptr);

    void setProducts(QVector<data::Product> products);
    const data::Product& productAt(int row) const;

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
    QVector<data::Product> m_products;
};

} // namespace ui
