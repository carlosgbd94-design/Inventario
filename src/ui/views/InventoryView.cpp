#include "ui/views/InventoryView.h"

#include <QCheckBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>
#include <QTableView>
#include <QUuid>
#include <QVBoxLayout>

#include "data/CategoryRepository.h"
#include "data/MovementRepository.h"
#include "data/ProductRepository.h"
#include "data/SupplierRepository.h"
#include "domain/InventoryEngine.h"
#include "domain/ReportEngine.h"
#include "ui/PdfExporter.h"
#include "ui/dialogs/MovementDialog.h"
#include "ui/dialogs/MovementHistoryDialog.h"
#include "ui/dialogs/ProductDialog.h"
#include "ui/models/ProductFilterProxyModel.h"
#include "ui/models/ProductTableModel.h"

namespace ui {

namespace {
// Copia el archivo elegido por el usuario a la carpeta de adjuntos de
// la app (junto a la base de datos) con un nombre unico, para que el
// archivo original pueda moverse o borrarse sin romper la referencia.
// Devuelve la ruta guardada, o cadena vacia si fallo la copia.
QString storeAttachment(const QString& sourcePath) {
    if (sourcePath.isEmpty()) {
        return {};
    }
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/attachments";
    QDir().mkpath(dir);

    const QFileInfo info(sourcePath);
    const QString destPath = QString("%1/%2_%3").arg(dir, QUuid::createUuid().toString(QUuid::Id128), info.fileName());

    if (!QFile::copy(sourcePath, destPath)) {
        return {};
    }
    return destPath;
}
} // namespace

InventoryView::InventoryView(data::CategoryRepository& categories, data::ProductRepository& products,
                              data::MovementRepository& movements, data::SupplierRepository& suppliers,
                              domain::InventoryEngine& engine, domain::ReportEngine& reportEngine, QWidget* parent)
    : QWidget(parent),
      m_categories(categories),
      m_products(products),
      m_movements(movements),
      m_suppliers(suppliers),
      m_engine(engine),
      m_reportEngine(reportEngine) {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(32, 28, 32, 28);
    rootLayout->setSpacing(16);

    auto* headerLayout = new QHBoxLayout();
    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName("TitleLabel");
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch(1);

    m_lowStockCheck = new QCheckBox("Solo bajo minimo", this);
    headerLayout->addWidget(m_lowStockCheck);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Buscar por codigo, producto o variante...");
    m_searchEdit->setFixedWidth(260);
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

    m_historyButton = new QPushButton("Historial", this);
    m_historyButton->setCursor(Qt::PointingHandCursor);
    toolbarLayout->addWidget(m_historyButton);

    m_deactivateButton = new QPushButton("Desactivar", this);
    m_deactivateButton->setCursor(Qt::PointingHandCursor);
    toolbarLayout->addWidget(m_deactivateButton);

    auto* exportButton = new QPushButton("Exportar PDF", this);
    exportButton->setCursor(Qt::PointingHandCursor);
    toolbarLayout->addWidget(exportButton);

    toolbarLayout->addStretch(1);
    rootLayout->addLayout(toolbarLayout);

    m_model = new ProductTableModel(this);
    m_proxy = new ProductFilterProxyModel(this);
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

    connect(m_searchEdit, &QLineEdit::textChanged, m_proxy, &ProductFilterProxyModel::setFilterFixedString);
    connect(m_lowStockCheck, &QCheckBox::toggled, m_proxy, &ProductFilterProxyModel::setLowStockOnly);
    connect(m_table, &QTableView::activated, this, &InventoryView::onEditActivated);
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &InventoryView::updateActionButtonsEnabled);
    connect(newButton, &QPushButton::clicked, this, &InventoryView::onNewProduct);
    connect(m_movementButton, &QPushButton::clicked, this, &InventoryView::onRegisterMovement);
    connect(m_historyButton, &QPushButton::clicked, this, &InventoryView::onViewHistory);
    connect(m_deactivateButton, &QPushButton::clicked, this, &InventoryView::onDeactivateSelected);
    connect(exportButton, &QPushButton::clicked, this, &InventoryView::onExportPdf);

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
    ProductDialog dialog(m_categories.all(), m_suppliers.all(), m_category.id, this);
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

    ProductDialog dialog(m_categories.all(), m_suppliers.all(), product.categoryId, this);
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

    domain::InventoryEngine::MovementInput input;
    input.productId = product.id;
    input.type = dialog.selectedType();
    input.quantity = dialog.quantity();
    input.note = dialog.note();

    const QString chosenAttachment = dialog.selectedAttachmentPath();
    if (!chosenAttachment.isEmpty()) {
        input.attachmentPath = storeAttachment(chosenAttachment);
        if (input.attachmentPath.isEmpty()) {
            QMessageBox::warning(this, "Movimiento", "No se pudo guardar el archivo adjunto; el movimiento no incluira adjunto.");
        } else {
            input.attachmentName = QFileInfo(chosenAttachment).fileName();
        }
    }

    const auto result = m_engine.registerMovement(input);
    if (!result.ok) {
        QMessageBox::warning(this, "Movimiento", result.error);
        return;
    }

    reload();
    emit inventoryChanged();
}

void InventoryView::onViewHistory() {
    const QModelIndexList selected = m_table->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        return;
    }
    const QModelIndex sourceIndex = m_proxy->mapToSource(selected.first());
    const data::Product product = m_model->productAt(sourceIndex.row());

    MovementHistoryDialog dialog(product, m_movements.byProduct(product.id), this);
    dialog.exec();
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

void InventoryView::onExportPdf() {
    const auto report = m_reportEngine.currentStockReport(m_category.id);
    const QString defaultName = QString("%1.pdf").arg(m_category.name);
    const QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString path = QFileDialog::getSaveFileName(this, "Exportar PDF", defaultDir + "/" + defaultName,
                                                        "Documento PDF (*.pdf)");
    if (path.isEmpty()) {
        return;
    }

    if (!exportReportToPdf(report, path)) {
        QMessageBox::warning(this, "Exportar PDF", "No se pudo generar el PDF.");
        return;
    }
    QMessageBox::information(this, "Exportar PDF", QString("%1 exportado correctamente.").arg(m_category.name));
}

void InventoryView::updateActionButtonsEnabled() {
    const bool hasSelection = m_table->selectionModel() && m_table->selectionModel()->hasSelection();
    m_movementButton->setEnabled(hasSelection);
    m_historyButton->setEnabled(hasSelection);
    m_deactivateButton->setEnabled(hasSelection);
}

} // namespace ui
