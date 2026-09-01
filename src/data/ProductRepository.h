#pragma once

#include <QSqlDatabase>
#include <QVector>
#include <optional>

#include "data/Models.h"

namespace data {

class ProductRepository {
public:
    explicit ProductRepository(QSqlDatabase& db);

    QVector<Product> all(bool activeOnly = true) const;
    QVector<Product> byCategory(qint64 categoryId, bool activeOnly = true) const;
    std::optional<Product> byId(qint64 id) const;

    qint64 insert(const Product& product);
    bool update(const Product& product);
    bool updateQuantity(qint64 productId, double newQty);
    bool setActive(qint64 productId, bool active);

private:
    QSqlDatabase& m_db;
};

} // namespace data
