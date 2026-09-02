#pragma once

#include <QString>
#include <QVector>
#include <optional>

class QSqlDatabase;

namespace data {
class ProductRepository;
class CategoryRepository;
class CutoffRepository;
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

// Arma los datos para un reporte de inventario (stock actual o un corte
// mensual congelado). El render a PDF vive aparte, en ui/PdfExporter.
class ReportEngine {
public:
    ReportEngine(data::ProductRepository& products, data::CategoryRepository& categories,
                 data::CutoffRepository& cutoffs);

    ReportData currentStockReport(std::optional<qint64> categoryId = std::nullopt) const;
    ReportData cutoffReport(qint64 cutoffId) const;

private:
    data::ProductRepository& m_products;
    data::CategoryRepository& m_categories;
    data::CutoffRepository& m_cutoffs;
};

} // namespace domain
