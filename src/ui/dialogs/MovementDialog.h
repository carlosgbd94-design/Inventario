#pragma once

#include <QDialog>

#include "data/Models.h"

class QComboBox;
class QDoubleSpinBox;
class QLineEdit;
class QLabel;
class QPushButton;
class QDragEnterEvent;
class QDropEvent;

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

    // Ruta local del archivo elegido (factura/ticket), vacia si no se
    // adjunto ninguno. InventoryView es quien lo copia a la carpeta de
    // adjuntos de la app al confirmar el movimiento.
    QString selectedAttachmentPath() const;

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    void onTypeChanged(int index);
    void setAttachment(const QString& path);

    data::Product m_product;
    QComboBox* m_typeCombo = nullptr;
    QLabel* m_qtyLabel = nullptr;
    QDoubleSpinBox* m_qtySpin = nullptr;
    QLineEdit* m_noteEdit = nullptr;
    QLabel* m_attachmentLabel = nullptr;
    QPushButton* m_clearAttachmentButton = nullptr;
    QString m_attachmentPath;
};

} // namespace ui
