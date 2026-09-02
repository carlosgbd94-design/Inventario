#pragma once

#include <QSqlDatabase>
#include <QVector>
#include <optional>

#include "data/Models.h"

namespace data {

class SupplierRepository {
public:
    explicit SupplierRepository(QSqlDatabase& db);

    QVector<Supplier> all(bool activeOnly = true) const;
    std::optional<Supplier> byId(qint64 id) const;

    qint64 insert(const Supplier& supplier);
    bool update(const Supplier& supplier);
    bool setActive(qint64 id, bool active);

private:
    QSqlDatabase& m_db;
};

} // namespace data
