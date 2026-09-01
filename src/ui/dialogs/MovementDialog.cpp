#include "ui/dialogs/MovementDialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

namespace ui {

MovementDialog::MovementDialog(const data::Product& product, QWidget* parent)
    : QDialog(parent), m_product(product) {
    setWindowTitle(QString("Movimiento - %1 %2").arg(product.name, product.variant).trimmed());
    setModal(true);
    setMinimumWidth(340);

    auto* rootLayout = new QVBoxLayout(this);

    auto* info = new QLabel(QString("Existencia actual: %1 %2").arg(product.currentQty).arg(product.unit), this);
    info->setObjectName("SubtitleLabel");
    rootLayout->addWidget(info);

    auto* form = new QFormLayout();
    form->setSpacing(10);

    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItem("Entrada", static_cast<int>(data::MovementType::Entrada));
    m_typeCombo->addItem("Salida", static_cast<int>(data::MovementType::Salida));
    m_typeCombo->addItem("Ajuste por recuento fisico", static_cast<int>(data::MovementType::Ajuste));
    form->addRow("Tipo", m_typeCombo);

    m_qtyLabel = new QLabel("Cantidad", this);
    m_qtySpin = new QDoubleSpinBox(this);
    m_qtySpin->setRange(0.0, 999999.0);
    m_qtySpin->setDecimals(2);
    form->addRow(m_qtyLabel, m_qtySpin);

    m_noteEdit = new QLineEdit(this);
    m_noteEdit->setPlaceholderText("Opcional");
    form->addRow("Nota", m_noteEdit);

    rootLayout->addLayout(form);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    QPushButton* okButton = buttons->button(QDialogButtonBox::Ok);
    okButton->setObjectName("PrimaryButton");
    okButton->style()->unpolish(okButton);
    okButton->style()->polish(okButton);
    rootLayout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MovementDialog::onTypeChanged);

    onTypeChanged(m_typeCombo->currentIndex());
}

void MovementDialog::onTypeChanged(int /*index*/) {
    const auto type = selectedType();
    if (type == data::MovementType::Ajuste) {
        m_qtyLabel->setText("Cantidad real contada");
        m_qtySpin->setValue(m_product.currentQty);
    } else {
        m_qtyLabel->setText("Cantidad");
        m_qtySpin->setValue(0.0);
    }
}

data::MovementType MovementDialog::selectedType() const {
    return static_cast<data::MovementType>(m_typeCombo->currentData().toInt());
}

double MovementDialog::quantity() const {
    return m_qtySpin->value();
}

QString MovementDialog::note() const {
    return m_noteEdit->text().trimmed();
}

} // namespace ui
