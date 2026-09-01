#include "domain/CutoffEngine.h"

#include <QDateTime>
#include <QHash>
#include <QSqlDatabase>

#include "data/CutoffRepository.h"
#include "data/ProductRepository.h"

namespace domain {

using data::CutoffSnapshotRow;
using data::MonthlyCutoff;
using data::Product;

CutoffEngine::CutoffEngine(QSqlDatabase& db, data::ProductRepository& products, data::CutoffRepository& cutoffs)
    : m_db(db), m_products(products), m_cutoffs(cutoffs) {}

CutoffEngine::CloseResult CutoffEngine::closeMonth(const QString& periodLabel, const QString& note) {
    if (m_cutoffs.byPeriod(periodLabel)) {
        return {false, "Ya existe un corte cerrado para " + periodLabel + ".", -1};
    }

    if (!m_db.transaction()) {
        return {false, "No se pudo iniciar la transaccion.", -1};
    }

    MonthlyCutoff cutoff;
    cutoff.period = periodLabel;
    cutoff.closedAt = QDateTime::currentDateTime();
    cutoff.note = note;

    const qint64 cutoffId = m_cutoffs.insertCutoff(cutoff);
    if (cutoffId < 0) {
        m_db.rollback();
        return {false, "No se pudo crear el corte.", -1};
    }

    for (const Product& product : m_products.all(true)) {
        CutoffSnapshotRow row;
        row.cutoffId = cutoffId;
        row.productId = product.id;
        row.quantity = product.currentQty;
        row.unitCost = product.unitCost;
        row.value = product.currentQty * product.unitCost;

        if (!m_cutoffs.insertSnapshotRow(row)) {
            m_db.rollback();
            return {false, "No se pudo guardar el detalle del corte.", -1};
        }
    }

    m_db.commit();
    return {true, {}, cutoffId};
}

QVector<CutoffEngine::ComparativeRow> CutoffEngine::compareWithPrevious(const QString& periodLabel) const {
    const QVector<MonthlyCutoff> cutoffs = m_cutoffs.all(); // ascendente por period

    int index = -1;
    for (int i = 0; i < cutoffs.size(); ++i) {
        if (cutoffs[i].period == periodLabel) {
            index = i;
            break;
        }
    }
    if (index <= 0) {
        return {};
    }

    const MonthlyCutoff& current = cutoffs[index];
    const MonthlyCutoff& previous = cutoffs[index - 1];

    QHash<qint64, double> previousQty;
    for (const CutoffSnapshotRow& row : m_cutoffs.snapshotForCutoff(previous.id)) {
        previousQty[row.productId] = row.quantity;
    }

    QVector<ComparativeRow> result;
    for (const CutoffSnapshotRow& row : m_cutoffs.snapshotForCutoff(current.id)) {
        const auto productOpt = m_products.byId(row.productId);
        if (!productOpt) {
            continue;
        }

        ComparativeRow comparative;
        comparative.product = *productOpt;
        comparative.currentQty = row.quantity;
        comparative.previousQty = previousQty.value(row.productId, 0.0);
        comparative.delta = comparative.currentQty - comparative.previousQty;
        result.push_back(comparative);
    }

    return result;
}

} // namespace domain
