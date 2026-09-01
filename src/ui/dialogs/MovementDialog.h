#pragma once

#include <QDialog>

#include "data/Models.h"

class QComboBox;
class QDoubleSpinBox;
class QLineEdit;
class QLabel;

namespace ui {

// Registro de un movimiento de stock (entrada, salida o ajuste por
// recuento fisico) para un producto puntual.
class MovementDialog : public QDialog {
    Q_OBJECT

public:
    explicit MovementDialog(const data::Product& product, QWidget* parent = nullptr);

    data::MovementType selectedType() const;
    double quantity() const;
    QString note() const;

private:
    void onTypeChanged(int index);

    data::Product m_product;
    QComboBox* m_typeCombo = nullptr;
    QLabel* m_qtyLabel = nullptr;
    QDoubleSpinBox* m_qtySpin = nullptr;
    QLineEdit* m_noteEdit = nullptr;
};

} // namespace ui
