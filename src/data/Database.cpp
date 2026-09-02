#include "data/Database.h"

#include <QDebug>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>

namespace data {

Database::Database(QString connectionName) : m_connectionName(std::move(connectionName)) {}

Database::~Database() {
    close();
}

QString Database::defaultDatabasePath() {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + "/inventario.db";
}

bool Database::open(const QString& filePath) {
    m_db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    m_db.setDatabaseName(filePath);

    if (!m_db.open()) {
        qWarning() << "Database::open failed:" << m_db.lastError().text();
        return false;
    }

    QSqlQuery pragma(m_db);
    if (!pragma.exec("PRAGMA foreign_keys = ON")) {
        qWarning() << "PRAGMA foreign_keys failed:" << pragma.lastError().text();
    }

    runMigrations();
    migrateToV2();
    seedDefaultCategories();

    return true;
}

void Database::close() {
    if (m_db.isOpen()) {
        m_db.close();
    }
    m_db = QSqlDatabase();
    QSqlDatabase::removeDatabase(m_connectionName);
}

QSqlDatabase& Database::handle() {
    return m_db;
}

namespace {
bool execOrWarn(QSqlQuery& query, const char* sql) {
    if (!query.exec(sql)) {
        qWarning() << "Migration statement failed:" << query.lastError().text() << "\nSQL:" << sql;
        return false;
    }
    return true;
}
} // namespace

void Database::runMigrations() {
    QSqlQuery query(m_db);

    execOrWarn(query, R"(
        CREATE TABLE IF NOT EXISTS schema_version (
            version INTEGER NOT NULL
        )
    )");

    execOrWarn(query, R"(
        CREATE TABLE IF NOT EXISTS category (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL UNIQUE,
            color TEXT NOT NULL DEFAULT ''
        )
    )");

    execOrWarn(query, R"(
        CREATE TABLE IF NOT EXISTS product (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            category_id INTEGER NOT NULL REFERENCES category(id),
            name TEXT NOT NULL,
            variant TEXT NOT NULL DEFAULT '',
            unit TEXT NOT NULL DEFAULT 'Pza',
            unit_cost REAL NOT NULL DEFAULT 0,
            current_qty REAL NOT NULL DEFAULT 0,
            min_stock REAL NOT NULL DEFAULT 0,
            active INTEGER NOT NULL DEFAULT 1
        )
    )");

    execOrWarn(query, R"(
        CREATE TABLE IF NOT EXISTS stock_movement (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            product_id INTEGER NOT NULL REFERENCES product(id),
            type TEXT NOT NULL,
            quantity REAL NOT NULL,
            date TEXT NOT NULL,
            note TEXT NOT NULL DEFAULT ''
        )
    )");

    execOrWarn(query, R"(
        CREATE TABLE IF NOT EXISTS monthly_cutoff (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            period TEXT NOT NULL UNIQUE,
            closed_at TEXT NOT NULL,
            note TEXT NOT NULL DEFAULT ''
        )
    )");

    execOrWarn(query, R"(
        CREATE TABLE IF NOT EXISTS cutoff_snapshot (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            cutoff_id INTEGER NOT NULL REFERENCES monthly_cutoff(id),
            product_id INTEGER NOT NULL REFERENCES product(id),
            quantity REAL NOT NULL,
            unit_cost REAL NOT NULL,
            value REAL NOT NULL
        )
    )");

    execOrWarn(query, "CREATE INDEX IF NOT EXISTS idx_product_category ON product(category_id)");
    execOrWarn(query, "CREATE INDEX IF NOT EXISTS idx_movement_product ON stock_movement(product_id)");
    execOrWarn(query, "CREATE INDEX IF NOT EXISTS idx_snapshot_cutoff ON cutoff_snapshot(cutoff_id)");
}

namespace {
bool tableHasColumn(QSqlDatabase& db, const QString& table, const QString& column) {
    QSqlQuery query(db);
    query.exec(QString("PRAGMA table_info(%1)").arg(table));
    while (query.next()) {
        if (query.value("name").toString().compare(column, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}
} // namespace

// v2: codigo/SKU y proveedor en producto, adjuntos en movimientos, y el
// catalogo de proveedores. Usa ALTER TABLE (no CREATE TABLE) porque
// bases de datos ya existentes con productos capturados no deben
// perder nada; PRAGMA table_info evita reintentar una columna que ya
// se agrego en una corrida anterior.
void Database::migrateToV2() {
    QSqlQuery query(m_db);

    if (!tableHasColumn(m_db, "product", "sku")) {
        execOrWarn(query, "ALTER TABLE product ADD COLUMN sku TEXT NOT NULL DEFAULT ''");
    }
    if (!tableHasColumn(m_db, "product", "supplier_id")) {
        execOrWarn(query, "ALTER TABLE product ADD COLUMN supplier_id INTEGER NOT NULL DEFAULT -1");
    }
    if (!tableHasColumn(m_db, "stock_movement", "attachment_path")) {
        execOrWarn(query, "ALTER TABLE stock_movement ADD COLUMN attachment_path TEXT NOT NULL DEFAULT ''");
    }
    if (!tableHasColumn(m_db, "stock_movement", "attachment_name")) {
        execOrWarn(query, "ALTER TABLE stock_movement ADD COLUMN attachment_name TEXT NOT NULL DEFAULT ''");
    }

    execOrWarn(query, R"(
        CREATE TABLE IF NOT EXISTS supplier (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            contact TEXT NOT NULL DEFAULT '',
            phone TEXT NOT NULL DEFAULT '',
            notes TEXT NOT NULL DEFAULT '',
            active INTEGER NOT NULL DEFAULT 1
        )
    )");
}

void Database::seedDefaultCategories() {
    QSqlQuery count(m_db);
    if (!count.exec("SELECT COUNT(*) FROM category")) {
        qWarning() << "seedDefaultCategories count failed:" << count.lastError().text();
        return;
    }
    if (count.next() && count.value(0).toInt() > 0) {
        return;
    }

    QSqlQuery insert(m_db);
    insert.prepare("INSERT INTO category (name, color) VALUES (?, ?)");

    insert.addBindValue("Uniformes");
    insert.addBindValue("#D79A52");
    if (!insert.exec()) {
        qWarning() << "seed Uniformes failed:" << insert.lastError().text();
    }

    insert.addBindValue("Papeleria");
    insert.addBindValue("#7C9CBF");
    if (!insert.exec()) {
        qWarning() << "seed Papeleria failed:" << insert.lastError().text();
    }
}

} // namespace data
