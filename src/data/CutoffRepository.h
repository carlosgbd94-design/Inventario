#pragma once

#include <QSqlDatabase>
#include <QVector>
#include <optional>

#include "data/Models.h"

namespace data {

class CutoffRepository {
public:
    explicit CutoffRepository(QSqlDatabase& db);

    QVector<MonthlyCutoff> all() const; // ordenado por period ascendente
    std::optional<MonthlyCutoff> byPeriod(const QString& period) const;

    qint64 insertCutoff(const MonthlyCutoff& cutoff);
    bool insertSnapshotRow(const CutoffSnapshotRow& row);
    QVector<CutoffSnapshotRow> snapshotForCutoff(qint64 cutoffId) const;

private:
    QSqlDatabase& m_db;
};

} // namespace data
