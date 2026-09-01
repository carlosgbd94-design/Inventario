#include "ui/views/InventoryView.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QTableView>
#include <QVBoxLayout>

#include "data/CategoryRepository.h"
#include "data/MovementRepository.h"
#include "data/ProductRepository.h"
#include "domain/InventoryEngine.h"
#include "ui/dialogs/MovementDialog.h"
#include "ui/dialogs/ProductDialog.h"
#include "ui/models/ProductTableModel.h"

namespace ui {

InventoryView::InventoryView(data::CategoryRepository& categories, data::ProductRepository& products,
                              data::MovementRepository& movements, domain::InventoryEngine& engine,
                              QWidget* parent)
    : QWidget(parent), m_categories(categories), m_products(products), m_movements(movements), m_engine(engine) {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(32, 28, 32, 28);
    rootLayout->setSpacing(16);

    auto* headerLayout = new QHBoxLayout();
    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName("TitleLabel");
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch(1);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Buscar producto...");
    m_searchEdit->setFixedWidth(220);
    headerLayout->addWidget(m_searchEdit);
    rootLayout->addLayout(headerLayout);

    auto* toolbarLayout = new QHBoxLayout();
    auto* newButton = new QPushButton("Nuevo producto", this);
    newButton->setObjectName("PrimaryButton");
    newButton->setCursor(Qt::PointingHandCursor);
    toolbarLayout->addWidget(newButton);

    m_movementButton = new QPushButton("Registrar movimiento", this);
    m_movementButton->setCursor(Qt::PointingHandCursor);
    toolbarLayout->addWidget(m_movementButton);

    m_deactivateButton = new QPushButton("Desactivar", this);
    m_deactivateButton->setCursor(Qt::PointingHandCursor);
    toolbarLayout->addWidget(m_deactivateButton);

    toolbarLayout->addStretch(1);
    rootLayout->addLayout(toolbarLayout);

    m_model = new ProductTableModel(this);
    m_proxy = new QSortFilterProxyModel(this);
    m_proxy->setSourceModel(m_model);
    m_proxy->setFilterKeyColumn(-1);
    m_proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);

    m_table = new QTableView(this);
    m_table->setModel(m_proxy);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(ProductTableModel::ColName, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(ProductTableModel::ColVariant, QHeaderView::Stretch);
    m_table->setSortingEnabled(true);
    rootLayout->addWidget(m_table, 1);

    connect(m_searchEdit, &QLineEdit::textChanged, m_proxy, &QSortFilterProxyModel::setFilterFixedString);
    connect(m_table, &QTableView::activated, this, &InventoryView::onEditActivated);
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &InventoryView::updateActionButtonsEnabled);
    connect(newButton, &QPushButton::clicked, this, &InventoryView::onNewProduct);
    connect(m_movementButton, &QPushButton::clicked, this, &InventoryView::onRegisterMovement);
    connect(m_deactivateButton, &QPushButton::clicked, this, &InventoryView::onDeactivateSelected);

    updateActionButtonsEnabled();
}

void InventoryView::setCategory(const data::Category& category) {
    m_category = category;
    m_titleLabel->setText(category.name);
    reload();
}

void InventoryView::reload() {
    m_model->setProducts(m_products.byCategory(m_category.id, true));
    updateActionButtonsEnabled();
}

void InventoryView::onNewProduct() {
    ProductDialog dialog(m_categories.all(), m_category.id, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    const data::Product product = dialog.resultProduct();
    if (m_products.insert(product) < 0) {
        QMessageBox::warning(this, "Producto", "No se pudo guardar el producto.");
        return;
    }

    reload();
    emit inventoryChanged();
}

void InventoryView::onEditActivated(const QModelIndex& proxyIndex) {
    const QModelIndex sourceIndex = m_proxy->mapToSource(proxyIndex);
    const data::Product product = m_model->productAt(sourceIndex.row());

    ProductDialog dialog(m_categories.all(), product.categoryId, this);
    dialog.loadProduct(product);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    data::Product updated = dialog.resultProduct();
    updated.currentQty = product.currentQty; // la cantidad solo cambia via movimientos
    if (!m_products.update(updated)) {
        QMessageBox::warning(this, "Producto", "No se pudo actualizar el producto.");
        return;
    }

    reload();
    emit inventoryChanged();
}

void InventoryView::onRegisterMovement() {
    const QModelIndexList selected = m_table->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        return;
    }
    const QModelIndex sourceIndex = m_proxy->mapToSource(selected.first());
    const data::Product product = m_model->productAt(sourceIndex.row());

    MovementDialog dialog(product, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    const auto result =
        m_engine.registerMovement(product.id, dialog.selectedType(), dialog.quantity(), dialog.note());
    if (!result.ok) {
        QMessageBox::warning(this, "Movimiento", result.error);
        return;
    }

    reload();
    emit inventoryChanged();
}

void InventoryView::onDeactivateSelected() {
    const QModelIndexList selected = m_table->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        return;
    }
    const QModelIndex sourceIndex = m_proxy->mapToSource(selected.first());
    const data::Product product = m_model->productAt(sourceIndex.row());

    const auto answer = QMessageBox::question(
        this, "Desactivar producto",
        QString("Desactivar \"%1 %2\"? Dejara de aparecer en el inventario activo.")
            .arg(product.name, product.variant));
    if (answer != QMessageBox::Yes) {
        return;
    }

    m_products.setActive(product.id, false);
    reload();
    emit inventoryChanged();
}

void InventoryView::updateActionButtonsEnabled() {
    const bool hasSelection = m_table->selectionModel() && m_table->selectionModel()->hasSelection();
    m_movementButton->setEnabled(hasSelection);
    m_deactivateButton->setEnabled(hasSelection);
}

} // namespace ui
