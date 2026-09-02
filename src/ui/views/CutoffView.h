#pragma once

#include <QWidget>

class QTableWidget;
class QPushButton;
class QLabel;
class QDateEdit;

namespace data {
class ProductRepository;
class CategoryRepository;
class CutoffRepository;
} // namespace data

namespace domain {
class CutoffEngine;
class ReportEngine;
} // namespace domain

namespace ui {

// Historial de cortes mensuales: cerrar el mes actual, ver el
// comparativo contra el corte anterior, exportar cualquiera de los dos
// (stock actual o un corte) a PDF, y exportar la bitacora de
// movimientos de un rango de fechas.
class CutoffView : public QWidget {
    Q_OBJECT

public:
    CutoffView(data::ProductRepository& products, data::CategoryRepository& categories,
               data::CutoffRepository& cutoffs, domain::CutoffEngine& cutoffEngine,
               domain::ReportEngine& reportEngine, QWidget* parent = nullptr);

    void reload();

signals:
    void cutoffsChanged();

private:
    void onCloseMonth();
    void onExportCurrentStock();
    void onExportSelectedCutoff();
    void onExportMovementReport();
    void onSelectionChanged();
    void reloadComparative(qint64 cutoffId);

    data::ProductRepository& m_products;
    data::CategoryRepository& m_categories;
    data::CutoffRepository& m_cutoffs;
    domain::CutoffEngine& m_cutoffEngine;
    domain::ReportEngine& m_reportEngine;

    QTableWidget* m_cutoffTable = nullptr;
    QTableWidget* m_comparativeTable = nullptr;
    QLabel* m_comparativeHint = nullptr;
    QPushButton* m_exportSelectedButton = nullptr;
    QDateEdit* m_movementFromDate = nullptr;
    QDateEdit* m_movementToDate = nullptr;
};

} // namespace ui
