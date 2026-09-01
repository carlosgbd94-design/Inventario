#include "domain/InventoryEngine.h"

#include <QDateTime>
#include <QSqlDatabase>

#include "data/MovementRepository.h"
#include "data/ProductRepository.h"

namespace domain {

using data::MovementType;
using data::Product;
using data::StockMovement;

InventoryEngine::InventoryEngine(QSqlDatabase& db, data::ProductRepository& products,
                                  data::MovementRepository& movements)
    : m_db(db), m_products(products), m_movements(movements) {}

InventoryEngine::MovementResult InventoryEngine::registerMovement(qint64 productId, MovementType type,
                                                                    double quantity, const QString& note) {
    const auto productOpt = m_products.byId(productId);
    if (!productOpt) {
        return {false, "El producto no existe.", 0.0};
    }
    const Product product = *productOpt;

    if (type != MovementType::Ajuste && quantity <= 0.0) {
        return {false, "La cantidad debe ser mayor que cero.", product.currentQty};
    }
    if (type == MovementType::Ajuste && quantity < 0.0) {
        return {false, "La cantidad ajustada no puede ser negativa.", product.currentQty};
    }

    double newQty = product.currentQty;
    switch (type) {
        case MovementType::Entrada:
            newQty = product.currentQty + quantity;
            break;
        case MovementType::Salida:
            newQty = product.currentQty - quantity;
            break;
        case MovementType::Ajuste:
            newQty = quantity;
            break;
    }

    if (newQty < 0.0) {
        return {false, "El movimiento dejaria el stock en negativo.", product.currentQty};
    }

    if (!m_db.transaction()) {
        return {false, "No se pudo iniciar la transaccion.", product.currentQty};
    }

    const double appliedDelta = newQty - product.currentQty;

    StockMovement movement;
    movement.productId = productId;
    movement.type = type;
    movement.quantity = appliedDelta;
    movement.date = QDateTime::currentDateTime();
    movement.note = note;

    const bool movementOk = m_movements.insert(movement) >= 0;
    const bool quantityOk = movementOk && m_products.updateQuantity(productId, newQty);

    if (!quantityOk) {
        m_db.rollback();
        return {false, "No se pudo registrar el movimiento.", product.currentQty};
    }

    m_db.commit();
    return {true, {}, newQty};
}

QVector<Product> InventoryEngine::reorderSuggestions(std::optional<qint64> categoryId) const {
    QVector<Product> candidates = categoryId ? m_products.byCategory(*categoryId, true) : m_products.all(true);

    QVector<Product> result;
    for (const Product& product : candidates) {
        if (product.minStock > 0.0 && product.currentQty <= product.minStock) {
            result.push_back(product);
        }
    }
    return result;
}

} // namespace domain
