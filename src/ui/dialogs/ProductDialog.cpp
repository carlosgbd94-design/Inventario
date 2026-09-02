#include "ui/dialogs/ProductDialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

#include "domain/SkuGenerator.h"

namespace ui {

namespace {
// Lista corta de unidades comunes en un almacen de uniformes/papeleria.
// El combo es editable: si el dato ya capturado no esta en la lista
// (ej. "Paquete 10 pzas"), se sigue mostrando tal cual.
const QStringList kStandardUnits = {"Pza", "Par", "Caja", "Paquete", "Bolsa",
                                     "Rollo", "Block", "Litro", "Kilogramo", "Metro", "Juego", "Kit"};
} // namespace

ProductDialog::ProductDialog(const QVector<data::Category>& categories, const QVector<data::Supplier>& suppliers,
                              qint64 defaultCategoryId, QSet<QString> existingSkus, QWidget* parent)
    : QDialog(parent), m_categories(categories), m_suppliers(suppliers), m_existingSkus(std::move(existingSkus)) {
    setWindowTitle("Producto");
    setModal(true);
    setMinimumWidth(360);

    auto* rootLayout = new QVBoxLayout(this);

    auto* form = new QFormLayout();
    form->setSpacing(10);

    m_categoryCombo = new QComboBox(this);
    int selectIndex = 0;
    for (int i = 0; i < m_categories.size(); ++i) {
        m_categoryCombo->addItem(m_categories[i].name, m_categories[i].id);
        if (m_categories[i].id == defaultCategoryId) {
            selectIndex = i;
        }
    }
    m_categoryCombo->setCurrentIndex(selectIndex);
    form->addRow("Categoria", m_categoryCombo);

    auto* skuLayout = new QHBoxLayout();
    m_skuEdit = new QLineEdit(this);
    m_skuEdit->setPlaceholderText("Codigo interno o de barras (opcional)");
    skuLayout->addWidget(m_skuEdit, 1);
    auto* generateSkuButton = new QPushButton("Generar", this);
    generateSkuButton->setCursor(Qt::PointingHandCursor);
    generateSkuButton->setToolTip("Arma un codigo a partir de la categoria, el nombre y la variante,\n"
                                   "asi no hay que inventar ni capturar numeros a mano.");
    skuLayout->addWidget(generateSkuButton);
    form->addRow("Codigo / SKU", skuLayout);

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText("Ej. Playera polo roja");
    form->addRow("Nombre", m_nameEdit);

    m_variantEdit = new QLineEdit(this);
    m_variantEdit->setPlaceholderText("Ej. Chica, Mediana, Carta...");
    form->addRow("Variante", m_variantEdit);

    m_unitCombo = new QComboBox(this);
    m_unitCombo->setEditable(true);
    m_unitCombo->addItems(kStandardUnits);
    m_unitCombo->setCurrentText("Pza");
    form->addRow("Unidad", m_unitCombo);

    m_costSpin = new QDoubleSpinBox(this);
    m_costSpin->setRange(0.0, 999999.0);
    m_costSpin->setDecimals(2);
    m_costSpin->setPrefix("$ ");
    m_costSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    form->addRow("Costo unitario", m_costSpin);

    m_qtySpin = new QDoubleSpinBox(this);
    m_qtySpin->setRange(0.0, 999999.0);
    m_qtySpin->setDecimals(2);
    m_qtySpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    form->addRow("Cantidad inicial", m_qtySpin);

    m_qtyHint = new QLabel(this);
    m_qtyHint->setObjectName("VersionLabel");
    m_qtyHint->setWordWrap(true);
    m_qtyHint->setVisible(false);
    form->addRow("", m_qtyHint);

    m_minStockSpin = new QDoubleSpinBox(this);
    m_minStockSpin->setRange(0.0, 999999.0);
    m_minStockSpin->setDecimals(2);
    m_minStockSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    form->addRow("Stock minimo (0 = sin alerta)", m_minStockSpin);

    m_supplierCombo = new QComboBox(this);
    m_supplierCombo->addItem("Sin proveedor", -1);
    for (const data::Supplier& supplier : m_suppliers) {
        m_supplierCombo->addItem(supplier.name, supplier.id);
    }
    form->addRow("Proveedor", m_supplierCombo);

    rootLayout->addLayout(form);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    QPushButton* okButton = buttons->button(QDialogButtonBox::Ok);
    okButton->setObjectName("PrimaryButton");
    // QDialogButtonBox ya "pulio" el boton con su estilo por defecto antes
    // de que pudieramos ponerle el objectName; hay que forzar que Qt vuelva
    // a evaluar el stylesheet para que tome el estilo de PrimaryButton.
    okButton->style()->unpolish(okButton);
    okButton->style()->polish(okButton);
    rootLayout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        if (m_nameEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "Producto", "El nombre del producto no puede estar vacio.");
            return;
        }
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(generateSkuButton, &QPushButton::clicked, this, &ProductDialog::generateSkuFromFields);
}

void ProductDialog::generateSkuFromFields() {
    const QString categoryName = m_categoryCombo->currentText();
    const QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::information(this, "Codigo / SKU", "Escribe primero el nombre del producto.");
        return;
    }
    const QString variant = m_variantEdit->text().trimmed();
    m_skuEdit->setText(domain::generateSku(categoryName, name, variant, m_existingSkus));
}

void ProductDialog::loadProduct(const data::Product& product) {
    m_productId = product.id;
    setWindowTitle("Editar producto");

    // El propio codigo del producto no cuenta como "choque" contra si
    // mismo si el usuario decide regenerarlo.
    m_existingSkus.remove(product.sku);

    const int index = m_categoryCombo->findData(product.categoryId);
    if (index >= 0) {
        m_categoryCombo->setCurrentIndex(index);
    }
    const int supplierIndex = m_supplierCombo->findData(product.supplierId);
    if (supplierIndex >= 0) {
        m_supplierCombo->setCurrentIndex(supplierIndex);
    }
    m_skuEdit->setText(product.sku);
    m_nameEdit->setText(product.name);
    m_variantEdit->setText(product.variant);
    m_unitCombo->setCurrentText(product.unit);
    m_costSpin->setValue(product.unitCost);
    m_minStockSpin->setValue(product.minStock);

    m_qtySpin->setValue(product.currentQty);
    m_qtySpin->setEnabled(false);
    m_qtyHint->setText("Para cambiar la cantidad usa \"Registrar movimiento\".");
    m_qtyHint->setVisible(true);
}

void ProductDialog::prefillFrom(const data::Product& product) {
    const int index = m_categoryCombo->findData(product.categoryId);
    if (index >= 0) {
        m_categoryCombo->setCurrentIndex(index);
    }
    const int supplierIndex = m_supplierCombo->findData(product.supplierId);
    if (supplierIndex >= 0) {
        m_supplierCombo->setCurrentIndex(supplierIndex);
    }
    m_nameEdit->setText(product.name);
    m_unitCombo->setCurrentText(product.unit);
    m_costSpin->setValue(product.unitCost);
    m_minStockSpin->setValue(product.minStock);
    m_variantEdit->setFocus();
}

data::Product ProductDialog::resultProduct() const {
    data::Product product;
    product.id = m_productId;
    product.categoryId = m_categoryCombo->currentData().toLongLong();
    product.supplierId = m_supplierCombo->currentData().toLongLong();
    product.sku = m_skuEdit->text().trimmed();
    product.name = m_nameEdit->text().trimmed();
    product.variant = m_variantEdit->text().trimmed();
    product.unit = m_unitCombo->currentText().trimmed().isEmpty() ? "Pza" : m_unitCombo->currentText().trimmed();
    product.unitCost = m_costSpin->value();
    product.currentQty = m_qtySpin->value();
    product.minStock = m_minStockSpin->value();
    product.active = true;
    return product;
}

} // namespace ui
