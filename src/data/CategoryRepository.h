#pragma once

#include <QSqlDatabase>
#include <QVector>
#include <optional>

#include "data/Models.h"

namespace data {

class CategoryRepository {
public:
    explicit CategoryRepository(QSqlDatabase& db);

    QVector<Category> all() const;
    std::optional<Category> byId(qint64 id) const;
    qint64 insert(const Category& category);
    bool update(const Category& category);

private:
    QSqlDatabase& m_db;
};

} // namespace data
