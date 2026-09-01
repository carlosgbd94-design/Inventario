#pragma once

#include <QDialog>
#include <QVector>

#include "data/Models.h"

class QComboBox;
class QLineEdit;
class QDoubleSpinBox;
class QLabel;

namespace ui {

// Alta o edicion de un producto. En edicion, la cantidad actual es de
// solo lectura: los cambios de stock se hacen por MovementDialog para
// que siempre queden en la bitacora de movimientos.
class ProductDialog : public QDialog {
    Q_OBJECT

public:
    ProductDialog(const QVector<data::Category>& categories, qint64 defaultCategoryId, QWidget* parent = nullptr);

    void loadProduct(const data::Product& product);
    data::Product resultProduct() const;

private:
    QComboBox* m_categoryCombo = nullptr;
    QLineEdit* m_nameEdit = nullptr;
    QLineEdit* m_variantEdit = nullptr;
    QLineEdit* m_unitEdit = nullptr;
    QDoubleSpinBox* m_costSpin = nullptr;
    QDoubleSpinBox* m_qtySpin = nullptr;
    QDoubleSpinBox* m_minStockSpin = nullptr;
    QLabel* m_qtyHint = nullptr;

    QVector<data::Category> m_categories;
    qint64 m_productId = -1;
};

} // namespace ui
