#pragma once

#include <QSqlDatabase>
#include <QString>

namespace data {

// Duenio de la conexion SQLite local de la app. Crea el esquema si no
// existe y siembra las categorias iniciales (Uniformes, Papeleria).
class Database {
public:
    explicit Database(QString connectionName);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    // filePath puede ser una ruta real o ":memory:" (usado en pruebas).
    bool open(const QString& filePath);
    void close();

    QSqlDatabase& handle();

    static QString defaultDatabasePath();

private:
    void runMigrations();
    void seedDefaultCategories();

    QString m_connectionName;
    QSqlDatabase m_db;
};

} // namespace data
