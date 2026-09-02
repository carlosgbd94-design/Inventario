#pragma once

#include <QDialog>
#include <QVector>

#include "data/Models.h"

class QTableWidget;

namespace ui {

// Bitacora de movimientos de un producto: fecha, tipo, cantidad, nota,
// y un boton para abrir el adjunto (factura/ticket) con el visor
// predeterminado de Windows si el movimiento tiene uno.
class MovementHistoryDialog : public QDialog {
    Q_OBJECT

public:
    MovementHistoryDialog(const data::Product& product, const QVector<data::StockMovement>& movements,
                           QWidget* parent = nullptr);

private:
    void onOpenAttachment(int row);

    QVector<data::StockMovement> m_movements;
    QTableWidget* m_table = nullptr;
};

} // namespace ui
