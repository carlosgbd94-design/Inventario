#pragma once

#include <QSqlDatabase>
#include <QVector>

#include "data/Models.h"

namespace data {

class MovementRepository {
public:
    explicit MovementRepository(QSqlDatabase& db);

    qint64 insert(const StockMovement& movement);
    QVector<StockMovement> byProduct(qint64 productId) const;
    QVector<StockMovement> inRange(const QDateTime& from, const QDateTime& to) const;
    bool hasMovements(qint64 productId) const;

private:
    QSqlDatabase& m_db;
};

} // namespace data
