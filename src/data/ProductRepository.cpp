#include "data/ProductRepository.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

namespace data {

namespace {
Product rowToProduct(const QSqlQuery& query) {
    Product product;
    product.id = query.value("id").toLongLong();
    product.categoryId = query.value("category_id").toLongLong();
    product.name = query.value("name").toString();
    product.variant = query.value("variant").toString();
    product.unit = query.value("unit").toString();
    product.unitCost = query.value("unit_cost").toDouble();
    product.currentQty = query.value("current_qty").toDouble();
    product.minStock = query.value("min_stock").toDouble();
    product.active = query.value("active").toInt() != 0;
    return product;
}

constexpr const char* kSelectColumns =
    "id, category_id, name, variant, unit, unit_cost, current_qty, min_stock, active";
} // namespace

ProductRepository::ProductRepository(QSqlDatabase& db) : m_db(db) {}

QVector<Product> ProductRepository::all(bool activeOnly) const {
    QVector<Product> result;
    QSqlQuery query(m_db);
    QString sql = QString("SELECT %1 FROM product").arg(kSelectColumns);
    if (activeOnly) {
        sql += " WHERE active = 1";
    }
    sql += " ORDER BY name, variant";
    query.exec(sql);
    while (query.next()) {
        result.push_back(rowToProduct(query));
    }
    return result;
}

QVector<Product> ProductRepository::byCategory(qint64 categoryId, bool activeOnly) const {
    QVector<Product> result;
    QSqlQuery query(m_db);
    QString sql = QString("SELECT %1 FROM product WHERE category_id = ?").arg(kSelectColumns);
    if (activeOnly) {
        sql += " AND active = 1";
    }
    sql += " ORDER BY name, variant";
    query.prepare(sql);
    query.addBindValue(categoryId);
    query.exec();
    while (query.next()) {
        result.push_back(rowToProduct(query));
    }
    return result;
}

std::optional<Product> ProductRepository::byId(qint64 id) const {
    QSqlQuery query(m_db);
    query.prepare(QString("SELECT %1 FROM product WHERE id = ?").arg(kSelectColumns));
    query.addBindValue(id);
    query.exec();
    if (query.next()) {
        return rowToProduct(query);
    }
    return std::nullopt;
}

qint64 ProductRepository::insert(const Product& product) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO product (category_id, name, variant, unit, unit_cost, current_qty, min_stock, active)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    )");
    query.addBindValue(product.categoryId);
    query.addBindValue(notNull(product.name));
    query.addBindValue(notNull(product.variant));
    query.addBindValue(notNull(product.unit));
    query.addBindValue(product.unitCost);
    query.addBindValue(product.currentQty);
    query.addBindValue(product.minStock);
    query.addBindValue(product.active ? 1 : 0);
    if (!query.exec()) {
        qWarning() << "ProductRepository::insert failed:" << query.lastError().text();
        return -1;
    }
    return query.lastInsertId().toLongLong();
}

bool ProductRepository::update(const Product& product) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        UPDATE product SET category_id = ?, name = ?, variant = ?, unit = ?,
            unit_cost = ?, min_stock = ?, active = ?
        WHERE id = ?
    )");
    query.addBindValue(product.categoryId);
    query.addBindValue(notNull(product.name));
    query.addBindValue(notNull(product.variant));
    query.addBindValue(notNull(product.unit));
    query.addBindValue(product.unitCost);
    query.addBindValue(product.minStock);
    query.addBindValue(product.active ? 1 : 0);
    query.addBindValue(product.id);
    return query.exec();
}

bool ProductRepository::updateQuantity(qint64 productId, double newQty) {
    QSqlQuery query(m_db);
    query.prepare("UPDATE product SET current_qty = ? WHERE id = ?");
    query.addBindValue(newQty);
    query.addBindValue(productId);
    return query.exec();
}

bool ProductRepository::setActive(qint64 productId, bool active) {
    QSqlQuery query(m_db);
    query.prepare("UPDATE product SET active = ? WHERE id = ?");
    query.addBindValue(active ? 1 : 0);
    query.addBindValue(productId);
    return query.exec();
}

} // namespace data
