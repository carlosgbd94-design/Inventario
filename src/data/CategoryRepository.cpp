#include "data/CategoryRepository.h"

#include <QSqlQuery>
#include <QSqlRecord>

namespace data {

namespace {
Category rowToCategory(const QSqlQuery& query) {
    Category category;
    category.id = query.value("id").toLongLong();
    category.name = query.value("name").toString();
    category.color = query.value("color").toString();
    return category;
}
} // namespace

CategoryRepository::CategoryRepository(QSqlDatabase& db) : m_db(db) {}

QVector<Category> CategoryRepository::all() const {
    QVector<Category> result;
    QSqlQuery query(m_db);
    query.exec("SELECT id, name, color FROM category ORDER BY name");
    while (query.next()) {
        result.push_back(rowToCategory(query));
    }
    return result;
}

std::optional<Category> CategoryRepository::byId(qint64 id) const {
    QSqlQuery query(m_db);
    query.prepare("SELECT id, name, color FROM category WHERE id = ?");
    query.addBindValue(id);
    query.exec();
    if (query.next()) {
        return rowToCategory(query);
    }
    return std::nullopt;
}

qint64 CategoryRepository::insert(const Category& category) {
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO category (name, color) VALUES (?, ?)");
    query.addBindValue(category.name);
    query.addBindValue(category.color);
    if (!query.exec()) {
        return -1;
    }
    return query.lastInsertId().toLongLong();
}

bool CategoryRepository::update(const Category& category) {
    QSqlQuery query(m_db);
    query.prepare("UPDATE category SET name = ?, color = ? WHERE id = ?");
    query.addBindValue(category.name);
    query.addBindValue(category.color);
    query.addBindValue(category.id);
    return query.exec();
}

} // namespace data
