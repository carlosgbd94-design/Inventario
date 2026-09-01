#pragma once

#include <QDateTime>
#include <QString>

namespace data {

// Nota: los campos QString se inicializan con "" (cadena vacia) y no con
// QString() (cadena nula) a proposito. QString() e "" no son lo mismo
// para Qt: bindear una QString nula en QSqlQuery la manda como SQL NULL,
// lo que revienta las columnas NOT NULL del esquema aunque el valor
// "logico" sea simplemente texto vacio.
struct Category {
    qint64 id = -1;
    QString name = "";
    QString color = "";
};

struct Product {
    qint64 id = -1;
    qint64 categoryId = -1;
    QString name = "";
    QString variant = "";
    QString unit = "";
    double unitCost = 0.0;
    double currentQty = 0.0;
    double minStock = 0.0;
    bool active = true;
};

enum class MovementType { Entrada, Salida, Ajuste };

QString movementTypeToString(MovementType type);
MovementType movementTypeFromString(const QString& value);

struct StockMovement {
    qint64 id = -1;
    qint64 productId = -1;
    MovementType type = MovementType::Entrada;
    double quantity = 0.0; // delta aplicado al stock (positivo = entro, negativo = salio)
    QDateTime date;
    QString note = "";
};

struct MonthlyCutoff {
    qint64 id = -1;
    QString period = ""; // "YYYY-MM"
    QDateTime closedAt;
    QString note = "";
};

struct CutoffSnapshotRow {
    qint64 id = -1;
    qint64 cutoffId = -1;
    qint64 productId = -1;
    double quantity = 0.0;
    double unitCost = 0.0;
    double value = 0.0;
};

} // namespace data
