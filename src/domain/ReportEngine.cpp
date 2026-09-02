#include "domain/ReportEngine.h"

#include "data/CategoryRepository.h"
#include "data/CutoffRepository.h"
#include "data/ProductRepository.h"

namespace domain {

using data::Category;
using data::CutoffSnapshotRow;
using data::MonthlyCutoff;
using data::Product;

ReportEngine::ReportEngine(data::ProductRepository& products, data::CategoryRepository& categories,
                            data::CutoffRepository& cutoffs)
    : m_products(products), m_categories(categories), m_cutoffs(cutoffs) {}

ReportData ReportEngine::currentStockReport(std::optional<qint64> categoryId) const {
    ReportData report;
    report.title = "Stock actual";

    if (categoryId) {
        const auto category = m_categories.byId(*categoryId);
        report.subtitle = category ? category->name : "Categoria";
    } else {
        report.subtitle = "Todas las categorias";
    }

    const QVector<Product> products = categoryId ? m_products.byCategory(*categoryId, true) : m_products.all(true);
    for (const Product& product : products) {
        ReportRow row;
        row.sku = product.sku;
        row.productName = product.name;
        row.variant = product.variant;
        row.unit = product.unit;
        row.quantity = product.currentQty;
        row.unitCost = product.unitCost;
        row.value = product.currentQty * product.unitCost;
        report.totalValue += row.value;
        report.rows.push_back(row);
    }

    return report;
}

ReportData ReportEngine::cutoffReport(qint64 cutoffId) const {
    ReportData report;
    report.title = "Corte mensual";
    report.subtitle = "Todas las categorias";

    for (const MonthlyCutoff& cutoff : m_cutoffs.all()) {
        if (cutoff.id == cutoffId) {
            report.subtitle = QString("Periodo %1 - cerrado el %2")
                                   .arg(cutoff.period, cutoff.closedAt.toString("dd/MM/yyyy"));
            break;
        }
    }

    for (const CutoffSnapshotRow& snapshot : m_cutoffs.snapshotForCutoff(cutoffId)) {
        ReportRow row;
        if (const auto product = m_products.byId(snapshot.productId)) {
            row.sku = product->sku;
            row.productName = product->name;
            row.variant = product->variant;
            row.unit = product->unit;
        } else {
            row.productName = "(producto eliminado)";
        }
        row.quantity = snapshot.quantity;
        row.unitCost = snapshot.unitCost;
        row.value = snapshot.value;
        report.totalValue += row.value;
        report.rows.push_back(row);
    }

    return report;
}

} // namespace domain
