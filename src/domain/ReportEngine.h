#pragma once

#include <QDateTime>
#include <QString>
#include <QVector>
#include <optional>

#include "data/Models.h"

class QSqlDatabase;

namespace data {
class ProductRepository;
class CategoryRepository;
class CutoffRepository;
class MovementRepository;
} // namespace data

namespace domain {

struct ReportRow {
    QString sku;
    QString productName;
    QString variant;
    QString unit;
    double quantity = 0.0;
    double unitCost = 0.0;
    double value = 0.0;
};

struct ReportData {
    QString title;
    QString subtitle;
    QVector<ReportRow> rows;
    double totalValue = 0.0;
};

struct MovementReportRow {
    QDateTime date;
    QString sku;
    QString productName;
    QString variant;
    data::MovementType type;
    double quantity = 0.0;
    QString note;
};

struct MovementReportData {
    QString title;
    QString subtitle;
    QVector<MovementReportRow> rows;
};

// Arma los datos para un reporte de inventario (stock actual, un corte
// mensual congelado, o la bitacora de movimientos en un rango de
// fechas). El render a PDF vive aparte, en ui/PdfExporter.
class ReportEngine {
public:
    ReportEngine(data::ProductRepository& products, data::CategoryRepository& categories,
                 data::CutoffRepository& cutoffs, data::MovementRepository& movements);

    ReportData currentStockReport(std::optional<qint64> categoryId = std::nullopt) const;
    ReportData cutoffReport(qint64 cutoffId) const;
    MovementReportData movementReport(const QDateTime& from, const QDateTime& to) const;

private:
    data::ProductRepository& m_products;
    data::CategoryRepository& m_categories;
    data::CutoffRepository& m_cutoffs;
    data::MovementRepository& m_movements;
};

} // namespace domain
