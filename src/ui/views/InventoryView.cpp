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
#include <QMenu>
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
#include "domain/SkuGenerator.h"
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

    m_editButton = new QPushButton("Editar", this);
    m_editButton->setCursor(Qt::PointingHandCursor);
    toolbarLayout->addWidget(m_editButton);

    m_movementButton = new QPushButton("Registrar movimiento", this);
    m_movementButton->setCursor(Qt::PointingHandCursor);
    toolbarLayout->addWidget(m_movementButton);

    m_historyButton = new QPushButton("Historial", this);
    m_historyButton->setCursor(Qt::PointingHandCursor);
    toolbarLayout->addWidget(m_historyButton);

    m_deactivateButton = new QPushButton("Desactivar", this);
    m_deactivateButton->setCursor(Qt::PointingHandCursor);
    toolbarLayout->addWidget(m_deactivateButton);

    m_deleteButton = new QPushButton("Eliminar", this);
    m_deleteButton->setCursor(Qt::PointingHandCursor);
    toolbarLayout->addWidget(m_deleteButton);

    auto* generateSkusButton = new QPushButton("Generar codigos faltantes", this);
    generateSkusButton->setCursor(Qt::PointingHandCursor);
    generateSkusButton->setToolTip("Asigna un codigo a todos los productos de esta categoria que aun no tienen uno.");
    toolbarLayout->addWidget(generateSkusButton);

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
    // Todas las columnas se ajustan al contenido y el usuario todavia
    // las puede arrastrar; solo "Producto" toma el espacio restante.
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_table->resizeColumnsToContents();
    m_table->setColumnWidth(ProductTableModel::ColSku, 130);
    m_table->setColumnWidth(ProductTableModel::ColVariant, 130);
    m_table->horizontalHeader()->setSectionResizeMode(ProductTableModel::ColName, QHeaderView::Stretch);
    m_table->setSortingEnabled(true);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);
    rootLayout->addWidget(m_table, 1);

    connect(m_searchEdit, &QLineEdit::textChanged, m_proxy, &ProductFilterProxyModel::setFilterFixedString);
    connect(m_lowStockCheck, &QCheckBox::toggled, m_proxy, &ProductFilterProxyModel::setLowStockOnly);
    connect(m_table, &QTableView::activated, this, &InventoryView::onEditActivated);
    connect(m_table, &QTableView::customContextMenuRequested, this, &InventoryView::showContextMenu);
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &InventoryView::updateActionButtonsEnabled);
    connect(newButton, &QPushButton::clicked, this, &InventoryView::onNewProduct);
    connect(m_editButton, &QPushButton::clicked, this, &InventoryView::onEditSelected);
    connect(m_movementButton, &QPushButton::clicked, this, &InventoryView::onRegisterMovement);
    connect(m_historyButton, &QPushButton::clicked, this, &InventoryView::onViewHistory);
    connect(m_deactivateButton, &QPushButton::clicked, this, &InventoryView::onDeactivateSelected);
    connect(m_deleteButton, &QPushButton::clicked, this, &InventoryView::onDeleteSelected);
    connect(generateSkusButton, &QPushButton::clicked, this, &InventoryView::onGenerateMissingSkus);
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
    ProductDialog dialog(m_categories.all(), m_suppliers.all(), m_category.id, existingSkus(), this);
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
    editProduct(m_model->productAt(sourceIndex.row()));
}

void InventoryView::onEditSelected() {
    if (const auto product = selectedProduct()) {
        editProduct(*product);
    }
}

void InventoryView::editProduct(const data::Product& product) {
    ProductDialog dialog(m_categories.all(), m_suppliers.all(), product.categoryId, existingSkus(), this);
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

std::optional<data::Product> InventoryView::selectedProduct() const {
    const QModelIndexList selected = m_table->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        return std::nullopt;
    }
    const QModelIndex sourceIndex = m_proxy->mapToSource(selected.first());
    return m_model->productAt(sourceIndex.row());
}

QSet<QString> InventoryView::existingSkus() const {
    QSet<QString> skus;
    for (const data::Product& product : m_products.all(false)) {
        if (!product.sku.isEmpty()) {
            skus.insert(product.sku);
        }
    }
    return skus;
}

void InventoryView::onRegisterMovement() {
    const auto productOpt = selectedProduct();
    if (!productOpt) {
        return;
    }
    const data::Product product = *productOpt;

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
    const auto productOpt = selectedProduct();
    if (!productOpt) {
        return;
    }
    MovementHistoryDialog dialog(*productOpt, m_movements.byProduct(productOpt->id), this);
    dialog.exec();
}

void InventoryView::onDeactivateSelected() {
    const auto productOpt = selectedProduct();
    if (!productOpt) {
        return;
    }
    const data::Product product = *productOpt;

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

void InventoryView::onDeleteSelected() {
    const auto productOpt = selectedProduct();
    if (!productOpt) {
        return;
    }
    const data::Product product = *productOpt;

    if (m_movements.hasMovements(product.id)) {
        QMessageBox::information(this, "Eliminar producto",
                                  QString("\"%1 %2\" ya tiene movimientos registrados; borrarlo perderia esa "
                                          "bitacora. Usa \"Desactivar\" para quitarlo del inventario activo "
                                          "sin perder el historial.")
                                      .arg(product.name, product.variant));
        return;
    }

    const auto answer = QMessageBox::question(
        this, "Eliminar producto",
        QString("Eliminar \"%1 %2\" definitivamente? Esta accion no se puede deshacer.")
            .arg(product.name, product.variant));
    if (answer != QMessageBox::Yes) {
        return;
    }

    if (!m_products.remove(product.id)) {
        QMessageBox::warning(this, "Eliminar producto", "No se pudo eliminar el producto.");
        return;
    }

    reload();
    emit inventoryChanged();
}

void InventoryView::onGenerateMissingSkus() {
    const QVector<data::Product> products = m_products.byCategory(m_category.id, true);
    QSet<QString> skus = existingSkus();
    QString categoryName = m_category.name;
    for (const data::Category& category : m_categories.all()) {
        if (category.id == m_category.id) {
            categoryName = category.name;
            break;
        }
    }

    int updated = 0;
    for (const data::Product& product : products) {
        if (!product.sku.isEmpty()) {
            continue;
        }
        data::Product withSku = product;
        withSku.sku = domain::generateSku(categoryName, product.name, product.variant, skus);
        skus.insert(withSku.sku);
        if (m_products.update(withSku)) {
            ++updated;
        }
    }

    if (updated == 0) {
        QMessageBox::information(this, "Generar codigos", "Todos los productos de esta categoria ya tienen codigo.");
        return;
    }

    reload();
    emit inventoryChanged();
    QMessageBox::information(this, "Generar codigos",
                              QString("Se generaron %1 codigo(s) nuevo(s).").arg(updated));
}

void InventoryView::showContextMenu(const QPoint& pos) {
    const QModelIndex index = m_table->indexAt(pos);
    if (index.isValid()) {
        m_table->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }
    if (!selectedProduct()) {
        return;
    }

    QMenu menu(this);
    menu.addAction("Editar", this, &InventoryView::onEditSelected);
    menu.addAction("Registrar movimiento", this, &InventoryView::onRegisterMovement);
    menu.addAction("Historial", this, &InventoryView::onViewHistory);
    menu.addSeparator();
    menu.addAction("Desactivar", this, &InventoryView::onDeactivateSelected);
    menu.addAction("Eliminar", this, &InventoryView::onDeleteSelected);
    menu.exec(m_table->viewport()->mapToGlobal(pos));
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
    m_editButton->setEnabled(hasSelection);
    m_movementButton->setEnabled(hasSelection);
    m_historyButton->setEnabled(hasSelection);
    m_deactivateButton->setEnabled(hasSelection);
    m_deleteButton->setEnabled(hasSelection);
}

} // namespace ui
