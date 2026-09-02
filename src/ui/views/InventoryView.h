#pragma once

#include <QSet>
#include <QWidget>
#include <optional>

#include "data/Models.h"

class QLabel;
class QLineEdit;
class QCheckBox;
class QPushButton;
class QTableView;

namespace data {
class CategoryRepository;
class ProductRepository;
class MovementRepository;
class SupplierRepository;
} // namespace data

namespace domain {
class InventoryEngine;
class ReportEngine;
} // namespace domain

namespace ui {

class ProductTableModel;
class ProductFilterProxyModel;

// Vista de inventario para una categoria: tabla de productos + alta,
// edicion y registro de movimientos.
class InventoryView : public QWidget {
    Q_OBJECT

public:
    InventoryView(data::CategoryRepository& categories, data::ProductRepository& products,
                  data::MovementRepository& movements, data::SupplierRepository& suppliers,
                  domain::InventoryEngine& engine, domain::ReportEngine& reportEngine, QWidget* parent = nullptr);

    void setCategory(const data::Category& category);
    void reload();

signals:
    void inventoryChanged();

private:
    void onNewProduct();
    void onEditActivated(const QModelIndex& proxyIndex);
    void onEditSelected();
    void editProduct(const data::Product& product);
    void onRegisterMovement();
    void onDeactivateSelected();
    void onDeleteSelected();
    void onGenerateMissingSkus();
    void onExportPdf();
    void onViewHistory();
    void updateActionButtonsEnabled();
    void showContextMenu(const QPoint& pos);
    std::optional<data::Product> selectedProduct() const;
    QSet<QString> existingSkus() const;

    data::CategoryRepository& m_categories;
    data::ProductRepository& m_products;
    data::MovementRepository& m_movements;
    data::SupplierRepository& m_suppliers;
    domain::InventoryEngine& m_engine;
    domain::ReportEngine& m_reportEngine;
    data::Category m_category;

    QLabel* m_titleLabel = nullptr;
    QLineEdit* m_searchEdit = nullptr;
    QCheckBox* m_lowStockCheck = nullptr;
    QPushButton* m_editButton = nullptr;
    QPushButton* m_movementButton = nullptr;
    QPushButton* m_historyButton = nullptr;
    QPushButton* m_deactivateButton = nullptr;
    QPushButton* m_deleteButton = nullptr;
    QTableView* m_table = nullptr;
    ProductTableModel* m_model = nullptr;
    ProductFilterProxyModel* m_proxy = nullptr;
};

} // namespace ui
