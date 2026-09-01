#include "data/MovementRepository.h"

#include <QSqlQuery>

namespace data {

MovementRepository::MovementRepository(QSqlDatabase& db) : m_db(db) {}

qint64 MovementRepository::insert(const StockMovement& movement) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO stock_movement (product_id, type, quantity, date, note)
        VALUES (?, ?, ?, ?, ?)
    )");
    query.addBindValue(movement.productId);
    query.addBindValue(movementTypeToString(movement.type));
    query.addBindValue(movement.quantity);
    query.addBindValue(movement.date.toString(Qt::ISODate));
    query.addBindValue(movement.note);
    if (!query.exec()) {
        return -1;
    }
    return query.lastInsertId().toLongLong();
}

QVector<StockMovement> MovementRepository::byProduct(qint64 productId) const {
    QVector<StockMovement> result;
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT id, product_id, type, quantity, date, note
        FROM stock_movement WHERE product_id = ? ORDER BY date DESC, id DESC
    )");
    query.addBindValue(productId);
    query.exec();
    while (query.next()) {
        StockMovement movement;
        movement.id = query.value("id").toLongLong();
        movement.productId = query.value("product_id").toLongLong();
        movement.type = movementTypeFromString(query.value("type").toString());
        movement.quantity = query.value("quantity").toDouble();
        movement.date = QDateTime::fromString(query.value("date").toString(), Qt::ISODate);
        movement.note = query.value("note").toString();
        result.push_back(movement);
    }
    return result;
}

} // namespace data
