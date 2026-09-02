#include "ui/dialogs/SupplierDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStyle>
#include <QTextEdit>
#include <QVBoxLayout>

namespace ui {

SupplierDialog::SupplierDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Proveedor");
    setModal(true);
    setMinimumWidth(360);

    auto* rootLayout = new QVBoxLayout(this);

    auto* form = new QFormLayout();
    form->setSpacing(10);

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText("Ej. Uniformes del Centro S.A.");
    form->addRow("Nombre", m_nameEdit);

    m_contactEdit = new QLineEdit(this);
    m_contactEdit->setPlaceholderText("Persona de contacto o correo");
    form->addRow("Contacto", m_contactEdit);

    m_phoneEdit = new QLineEdit(this);
    m_phoneEdit->setPlaceholderText("Telefono");
    form->addRow("Telefono", m_phoneEdit);

    m_notesEdit = new QTextEdit(this);
    m_notesEdit->setPlaceholderText("Notas (opcional)");
    m_notesEdit->setFixedHeight(80);
    form->addRow("Notas", m_notesEdit);

    rootLayout->addLayout(form);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    QPushButton* okButton = buttons->button(QDialogButtonBox::Ok);
    okButton->setObjectName("PrimaryButton");
    okButton->style()->unpolish(okButton);
    okButton->style()->polish(okButton);
    rootLayout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        if (m_nameEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "Proveedor", "El nombre del proveedor no puede estar vacio.");
            return;
        }
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void SupplierDialog::loadSupplier(const data::Supplier& supplier) {
    m_supplierId = supplier.id;
    setWindowTitle("Editar proveedor");
    m_nameEdit->setText(supplier.name);
    m_contactEdit->setText(supplier.contact);
    m_phoneEdit->setText(supplier.phone);
    m_notesEdit->setPlainText(supplier.notes);
}

data::Supplier SupplierDialog::resultSupplier() const {
    data::Supplier supplier;
    supplier.id = m_supplierId;
    supplier.name = m_nameEdit->text().trimmed();
    supplier.contact = m_contactEdit->text().trimmed();
    supplier.phone = m_phoneEdit->text().trimmed();
    supplier.notes = m_notesEdit->toPlainText().trimmed();
    supplier.active = true;
    return supplier;
}

} // namespace ui
