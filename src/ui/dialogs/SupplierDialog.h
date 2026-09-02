#pragma once

#include <QDialog>

#include "data/Models.h"

class QLineEdit;
class QTextEdit;

namespace ui {

// Alta o edicion de un proveedor.
class SupplierDialog : public QDialog {
    Q_OBJECT

public:
    explicit SupplierDialog(QWidget* parent = nullptr);

    void loadSupplier(const data::Supplier& supplier);
    data::Supplier resultSupplier() const;

private:
    QLineEdit* m_nameEdit = nullptr;
    QLineEdit* m_contactEdit = nullptr;
    QLineEdit* m_phoneEdit = nullptr;
    QTextEdit* m_notesEdit = nullptr;
    qint64 m_supplierId = -1;
};

} // namespace ui
