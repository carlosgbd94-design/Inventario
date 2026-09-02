#include "ui/views/SupplierView.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include "data/SupplierRepository.h"
#include "ui/dialogs/SupplierDialog.h"

namespace ui {

namespace {
constexpr int kRoleSupplierId = Qt::UserRole;

QTableWidgetItem* readOnlyItem(const QString& text) {
    auto* item = new QTableWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    return item;
}
} // namespace

SupplierView::SupplierView(data::SupplierRepository& suppliers, QWidget* parent)
    : QWidget(parent), m_suppliers(suppliers) {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(32, 28, 32, 28);
    rootLayout->setSpacing(16);

    auto* title = new QLabel("Proveedores", this);
    title->setObjectName("TitleLabel");
    rootLayout->addWidget(title);

    auto* toolbarLayout = new QHBoxLayout();
    auto* newButton = new QPushButton("Nuevo proveedor", this);
    newButton->setObjectName("PrimaryButton");
    newButton->setCursor(Qt::PointingHandCursor);
    toolbarLayout->addWidget(newButton);

    m_deactivateButton = new QPushButton("Desactivar", this);
    m_deactivateButton->setCursor(Qt::PointingHandCursor);
    toolbarLayout->addWidget(m_deactivateButton);

    toolbarLayout->addStretch(1);
    rootLayout->addLayout(toolbarLayout);

    m_table = new QTableWidget(0, 4, this);
    m_table->setHorizontalHeaderLabels({"Nombre", "Contacto", "Telefono", "Notas"});
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    rootLayout->addWidget(m_table, 1);

    connect(newButton, &QPushButton::clicked, this, &SupplierView::onNew);
    connect(m_deactivateButton, &QPushButton::clicked, this, &SupplierView::onDeactivateSelected);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &SupplierView::onEditActivated);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &SupplierView::updateActionButtonsEnabled);

    reload();
}

void SupplierView::reload() {
    const auto suppliers = m_suppliers.all();
    m_table->setRowCount(0);
    for (const data::Supplier& supplier : suppliers) {
        const int row = m_table->rowCount();
        m_table->insertRow(row);

        auto* nameItem = readOnlyItem(supplier.name);
        nameItem->setData(kRoleSupplierId, supplier.id);
        m_table->setItem(row, 0, nameItem);
        m_table->setItem(row, 1, readOnlyItem(supplier.contact));
        m_table->setItem(row, 2, readOnlyItem(supplier.phone));
        m_table->setItem(row, 3, readOnlyItem(supplier.notes));
    }
    updateActionButtonsEnabled();
}

void SupplierView::onNew() {
    SupplierDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    if (m_suppliers.insert(dialog.resultSupplier()) < 0) {
        QMessageBox::warning(this, "Proveedor", "No se pudo guardar el proveedor.");
        return;
    }
    reload();
    emit suppliersChanged();
}

void SupplierView::onEditActivated(int row, int /*column*/) {
    const qint64 supplierId = m_table->item(row, 0)->data(kRoleSupplierId).toLongLong();
    const auto supplierOpt = m_suppliers.byId(supplierId);
    if (!supplierOpt) {
        return;
    }

    SupplierDialog dialog(this);
    dialog.loadSupplier(*supplierOpt);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    if (!m_suppliers.update(dialog.resultSupplier())) {
        QMessageBox::warning(this, "Proveedor", "No se pudo actualizar el proveedor.");
        return;
    }
    reload();
    emit suppliersChanged();
}

void SupplierView::onDeactivateSelected() {
    const auto selected = m_table->selectedItems();
    if (selected.isEmpty()) {
        return;
    }
    const int row = selected.first()->row();
    const qint64 supplierId = m_table->item(row, 0)->data(kRoleSupplierId).toLongLong();
    const QString name = m_table->item(row, 0)->text();

    const auto answer = QMessageBox::question(this, "Desactivar proveedor",
                                                QString("Desactivar \"%1\"? Dejara de aparecer al asignar productos.").arg(name));
    if (answer != QMessageBox::Yes) {
        return;
    }

    m_suppliers.setActive(supplierId, false);
    reload();
    emit suppliersChanged();
}

void SupplierView::updateActionButtonsEnabled() {
    m_deactivateButton->setEnabled(m_table->selectionModel() && m_table->selectionModel()->hasSelection());
}

} // namespace ui
