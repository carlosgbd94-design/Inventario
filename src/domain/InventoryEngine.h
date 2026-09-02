#pragma once

#include <QSqlDatabase>
#include <QString>
#include <QVector>
#include <optional>

#include "data/Models.h"

namespace data {
class ProductRepository;
class MovementRepository;
} // namespace data

namespace domain {

// Motor de reglas de negocio del inventario: valida y aplica movimientos
// de stock, y calcula sugerencias de reorden. No conoce nada de UI.
class InventoryEngine {
public:
    InventoryEngine(QSqlDatabase& db, data::ProductRepository& products,
                     data::MovementRepository& movements);

    struct MovementResult {
        bool ok = false;
        QString error;
        double newQuantity = 0.0;
    };

    struct MovementInput {
        qint64 productId = -1;
        data::MovementType type = data::MovementType::Entrada;
        // Para Entrada/Salida, `quantity` es la cantidad del movimiento
        // (>0). Para Ajuste, es la cantidad final absoluta despues de una
        // recuenta fisica (>=0).
        double quantity = 0.0;
        QString note = "";
        // Ruta y nombre del archivo (factura/ticket) adjunto al
        // movimiento, si el usuario adjunto uno. Vacios si no hay.
        QString attachmentPath = "";
        QString attachmentName = "";
    };

    MovementResult registerMovement(const MovementInput& input);

    // Productos activos cuyo stock actual esta en o por debajo de su
    // stock minimo configurado (min_stock > 0 para ser considerado).
    QVector<data::Product> reorderSuggestions(std::optional<qint64> categoryId = std::nullopt) const;

private:
    QSqlDatabase& m_db;
    data::ProductRepository& m_products;
    data::MovementRepository& m_movements;
};

} // namespace domain
