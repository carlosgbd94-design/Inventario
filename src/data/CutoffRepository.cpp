#include "data/CutoffRepository.h"

#include <QSqlQuery>

namespace data {

namespace {
MonthlyCutoff rowToCutoff(const QSqlQuery& query) {
    MonthlyCutoff cutoff;
    cutoff.id = query.value("id").toLongLong();
    cutoff.period = query.value("period").toString();
    cutoff.closedAt = QDateTime::fromString(query.value("closed_at").toString(), Qt::ISODate);
    cutoff.note = query.value("note").toString();
    return cutoff;
}
} // namespace

CutoffRepository::CutoffRepository(QSqlDatabase& db) : m_db(db) {}

QVector<MonthlyCutoff> CutoffRepository::all() const {
    QVector<MonthlyCutoff> result;
    QSqlQuery query(m_db);
    query.exec("SELECT id, period, closed_at, note FROM monthly_cutoff ORDER BY period ASC");
    while (query.next()) {
        result.push_back(rowToCutoff(query));
    }
    return result;
}

std::optional<MonthlyCutoff> CutoffRepository::byPeriod(const QString& period) const {
    QSqlQuery query(m_db);
    query.prepare("SELECT id, period, closed_at, note FROM monthly_cutoff WHERE period = ?");
    query.addBindValue(period);
    query.exec();
    if (query.next()) {
        return rowToCutoff(query);
    }
    return std::nullopt;
}

qint64 CutoffRepository::insertCutoff(const MonthlyCutoff& cutoff) {
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO monthly_cutoff (period, closed_at, note) VALUES (?, ?, ?)");
    query.addBindValue(cutoff.period);
    query.addBindValue(cutoff.closedAt.toString(Qt::ISODate));
    query.addBindValue(cutoff.note);
    if (!query.exec()) {
        return -1;
    }
    return query.lastInsertId().toLongLong();
}

bool CutoffRepository::insertSnapshotRow(const CutoffSnapshotRow& row) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO cutoff_snapshot (cutoff_id, product_id, quantity, unit_cost, value)
        VALUES (?, ?, ?, ?, ?)
    )");
    query.addBindValue(row.cutoffId);
    query.addBindValue(row.productId);
    query.addBindValue(row.quantity);
    query.addBindValue(row.unitCost);
    query.addBindValue(row.value);
    return query.exec();
}

QVector<CutoffSnapshotRow> CutoffRepository::snapshotForCutoff(qint64 cutoffId) const {
    QVector<CutoffSnapshotRow> result;
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT id, cutoff_id, product_id, quantity, unit_cost, value
        FROM cutoff_snapshot WHERE cutoff_id = ?
    )");
    query.addBindValue(cutoffId);
    query.exec();
    while (query.next()) {
        CutoffSnapshotRow row;
        row.id = query.value("id").toLongLong();
        row.cutoffId = query.value("cutoff_id").toLongLong();
        row.productId = query.value("product_id").toLongLong();
        row.quantity = query.value("quantity").toDouble();
        row.unitCost = query.value("unit_cost").toDouble();
        row.value = query.value("value").toDouble();
        result.push_back(row);
    }
    return result;
}

} // namespace data
