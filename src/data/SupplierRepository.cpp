#include "data/SupplierRepository.h"

#include <QSqlQuery>

namespace data {

namespace {
Supplier rowToSupplier(const QSqlQuery& query) {
    Supplier supplier;
    supplier.id = query.value("id").toLongLong();
    supplier.name = query.value("name").toString();
    supplier.contact = query.value("contact").toString();
    supplier.phone = query.value("phone").toString();
    supplier.notes = query.value("notes").toString();
    supplier.active = query.value("active").toInt() != 0;
    return supplier;
}

constexpr const char* kSelectColumns = "id, name, contact, phone, notes, active";
} // namespace

SupplierRepository::SupplierRepository(QSqlDatabase& db) : m_db(db) {}

QVector<Supplier> SupplierRepository::all(bool activeOnly) const {
    QVector<Supplier> result;
    QSqlQuery query(m_db);
    QString sql = QString("SELECT %1 FROM supplier").arg(kSelectColumns);
    if (activeOnly) {
        sql += " WHERE active = 1";
    }
    sql += " ORDER BY name";
    query.exec(sql);
    while (query.next()) {
        result.push_back(rowToSupplier(query));
    }
    return result;
}

std::optional<Supplier> SupplierRepository::byId(qint64 id) const {
    QSqlQuery query(m_db);
    query.prepare(QString("SELECT %1 FROM supplier WHERE id = ?").arg(kSelectColumns));
    query.addBindValue(id);
    query.exec();
    if (query.next()) {
        return rowToSupplier(query);
    }
    return std::nullopt;
}

qint64 SupplierRepository::insert(const Supplier& supplier) {
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO supplier (name, contact, phone, notes, active) VALUES (?, ?, ?, ?, ?)");
    query.addBindValue(notNull(supplier.name));
    query.addBindValue(notNull(supplier.contact));
    query.addBindValue(notNull(supplier.phone));
    query.addBindValue(notNull(supplier.notes));
    query.addBindValue(supplier.active ? 1 : 0);
    if (!query.exec()) {
        return -1;
    }
    return query.lastInsertId().toLongLong();
}

bool SupplierRepository::update(const Supplier& supplier) {
    QSqlQuery query(m_db);
    query.prepare("UPDATE supplier SET name = ?, contact = ?, phone = ?, notes = ? WHERE id = ?");
    query.addBindValue(notNull(supplier.name));
    query.addBindValue(notNull(supplier.contact));
    query.addBindValue(notNull(supplier.phone));
    query.addBindValue(notNull(supplier.notes));
    query.addBindValue(supplier.id);
    return query.exec();
}

bool SupplierRepository::setActive(qint64 id, bool active) {
    QSqlQuery query(m_db);
    query.prepare("UPDATE supplier SET active = ? WHERE id = ?");
    query.addBindValue(active ? 1 : 0);
    query.addBindValue(id);
    return query.exec();
}

} // namespace data
