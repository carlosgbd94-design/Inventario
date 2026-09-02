#pragma once

#include <QWidget>

class QTableWidget;
class QPushButton;

namespace data {
class SupplierRepository;
}

namespace ui {

// Catalogo de proveedores: alta, edicion y baja. Los productos se
// pueden asociar a un proveedor desde ProductDialog.
class SupplierView : public QWidget {
    Q_OBJECT

public:
    explicit SupplierView(data::SupplierRepository& suppliers, QWidget* parent = nullptr);

    void reload();

signals:
    void suppliersChanged();

private:
    void onNew();
    void onEditActivated(int row, int column);
    void onDeactivateSelected();
    void updateActionButtonsEnabled();

    data::SupplierRepository& m_suppliers;

    QTableWidget* m_table = nullptr;
    QPushButton* m_deactivateButton = nullptr;
};

} // namespace ui
