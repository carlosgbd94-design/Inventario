#pragma once

#include <QSqlDatabase>
#include <QString>
#include <QVector>

#include "data/Models.h"

namespace data {
class ProductRepository;
class CutoffRepository;
} // namespace data

namespace domain {

// Cierra los cortes mensuales de inventario: congela un snapshot de todo
// el stock activo y permite comparar un corte contra el anterior.
class CutoffEngine {
public:
    CutoffEngine(QSqlDatabase& db, data::ProductRepository& products, data::CutoffRepository& cutoffs);

    struct CloseResult {
        bool ok = false;
        QString error;
        qint64 cutoffId = -1;
    };

    // periodLabel esperado en formato "YYYY-MM". Falla si ya existe un
    // corte cerrado para ese periodo.
    CloseResult closeMonth(const QString& periodLabel, const QString& note = QString(""));

    struct ComparativeRow {
        data::Product product;
        double previousQty = 0.0;
        double currentQty = 0.0;
        double delta = 0.0;
    };

    // Compara el corte de `periodLabel` contra el corte inmediatamente
    // anterior (por orden alfabetico de periodo). Lista vacia si no hay
    // corte anterior o el periodo no existe.
    QVector<ComparativeRow> compareWithPrevious(const QString& periodLabel) const;

private:
    QSqlDatabase& m_db;
    data::ProductRepository& m_products;
    data::CutoffRepository& m_cutoffs;
};

} // namespace domain
