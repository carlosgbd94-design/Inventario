#include "ui/views/CutoffView.h"

#include <QDate>
#include <QDateEdit>
#include <QDateTime>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QColor>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>
#include <QTableWidget>
#include <QVBoxLayout>

#include "data/CategoryRepository.h"
#include "data/CutoffRepository.h"
#include "data/ProductRepository.h"
#include "domain/CutoffEngine.h"
#include "domain/ReportEngine.h"
#include "ui/PdfExporter.h"

namespace ui {

namespace {
constexpr int kRoleCutoffId = Qt::UserRole;

QTableWidgetItem* readOnlyItem(const QString& text) {
    auto* item = new QTableWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    return item;
}
} // namespace

CutoffView::CutoffView(data::ProductRepository& products, data::CategoryRepository& categories,
                        data::CutoffRepository& cutoffs, domain::CutoffEngine& cutoffEngine,
                        domain::ReportEngine& reportEngine, QWidget* parent)
    : QWidget(parent),
      m_products(products),
      m_categories(categories),
      m_cutoffs(cutoffs),
      m_cutoffEngine(cutoffEngine),
      m_reportEngine(reportEngine) {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(32, 28, 32, 28);
    rootLayout->setSpacing(16);

    auto* title = new QLabel("Cortes mensuales", this);
    title->setObjectName("TitleLabel");
    rootLayout->addWidget(title);

    auto* toolbarLayout = new QHBoxLayout();
    auto* closeMonthButton = new QPushButton("Cerrar mes actual", this);
    closeMonthButton->setObjectName("PrimaryButton");
    closeMonthButton->setCursor(Qt::PointingHandCursor);
    toolbarLayout->addWidget(closeMonthButton);

    auto* exportCurrentButton = new QPushButton("Exportar stock actual (PDF)", this);
    exportCurrentButton->setCursor(Qt::PointingHandCursor);
    toolbarLayout->addWidget(exportCurrentButton);
    toolbarLayout->addStretch(1);
    rootLayout->addLayout(toolbarLayout);

    m_cutoffTable = new QTableWidget(0, 3, this);
    m_cutoffTable->setHorizontalHeaderLabels({"Periodo", "Cerrado el", "Valor total"});
    m_cutoffTable->verticalHeader()->setVisible(false);
    m_cutoffTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_cutoffTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_cutoffTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_cutoffTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_cutoffTable->setMaximumHeight(220);
    rootLayout->addWidget(m_cutoffTable);

    auto* comparativeLayout = new QHBoxLayout();
    auto* comparativeTitle = new QLabel("Comparativo contra el corte anterior", this);
    comparativeTitle->setObjectName("SubtitleLabel");
    comparativeLayout->addWidget(comparativeTitle);
    comparativeLayout->addStretch(1);
    m_exportSelectedButton = new QPushButton("Exportar corte seleccionado (PDF)", this);
    m_exportSelectedButton->setCursor(Qt::PointingHandCursor);
    m_exportSelectedButton->setEnabled(false);
    comparativeLayout->addWidget(m_exportSelectedButton);
    rootLayout->addLayout(comparativeLayout);

    m_comparativeHint = new QLabel("Selecciona un corte para ver su comparativo.", this);
    m_comparativeHint->setObjectName("VersionLabel");
    rootLayout->addWidget(m_comparativeHint);

    m_comparativeTable = new QTableWidget(0, 5, this);
    m_comparativeTable->setHorizontalHeaderLabels({"Producto", "Variante", "Cant. anterior", "Cant. actual", "Diferencia"});
    m_comparativeTable->verticalHeader()->setVisible(false);
    m_comparativeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_comparativeTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_comparativeTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_comparativeTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    rootLayout->addWidget(m_comparativeTable, 1);

    auto* movementTitle = new QLabel("Reporte de movimientos por periodo", this);
    movementTitle->setObjectName("SubtitleLabel");
    rootLayout->addWidget(movementTitle);

    auto* movementLayout = new QHBoxLayout();
    movementLayout->addWidget(new QLabel("Desde", this));
    m_movementFromDate = new QDateEdit(QDate::currentDate().addDays(-30), this);
    m_movementFromDate->setCalendarPopup(true);
    m_movementFromDate->setDisplayFormat("dd/MM/yyyy");
    movementLayout->addWidget(m_movementFromDate);

    movementLayout->addWidget(new QLabel("Hasta", this));
    m_movementToDate = new QDateEdit(QDate::currentDate(), this);
    m_movementToDate->setCalendarPopup(true);
    m_movementToDate->setDisplayFormat("dd/MM/yyyy");
    movementLayout->addWidget(m_movementToDate);

    auto* exportMovementsButton = new QPushButton("Exportar movimientos (PDF)", this);
    exportMovementsButton->setCursor(Qt::PointingHandCursor);
    movementLayout->addWidget(exportMovementsButton);
    movementLayout->addStretch(1);
    rootLayout->addLayout(movementLayout);

    connect(closeMonthButton, &QPushButton::clicked, this, &CutoffView::onCloseMonth);
    connect(exportCurrentButton, &QPushButton::clicked, this, &CutoffView::onExportCurrentStock);
    connect(m_exportSelectedButton, &QPushButton::clicked, this, &CutoffView::onExportSelectedCutoff);
    connect(m_cutoffTable, &QTableWidget::itemSelectionChanged, this, &CutoffView::onSelectionChanged);
    connect(exportMovementsButton, &QPushButton::clicked, this, &CutoffView::onExportMovementReport);

    reload();
}

void CutoffView::reload() {
    const auto cutoffs = m_cutoffs.all();
    const QLocale locale(QLocale::Spanish, QLocale::Mexico);

    m_cutoffTable->setRowCount(0);
    for (int i = cutoffs.size() - 1; i >= 0; --i) {
        const data::MonthlyCutoff& cutoff = cutoffs[i];
        const int row = m_cutoffTable->rowCount();
        m_cutoffTable->insertRow(row);

        auto* periodItem = readOnlyItem(cutoff.period);
        periodItem->setData(kRoleCutoffId, cutoff.id);
        m_cutoffTable->setItem(row, 0, periodItem);
        m_cutoffTable->setItem(row, 1, readOnlyItem(cutoff.closedAt.toString("dd/MM/yyyy hh:mm")));

        const double total = m_reportEngine.cutoffReport(cutoff.id).totalValue;
        m_cutoffTable->setItem(row, 2, readOnlyItem(locale.toCurrencyString(total, "$")));
    }

    m_comparativeTable->setRowCount(0);
    m_exportSelectedButton->setEnabled(false);
    m_comparativeHint->setText(cutoffs.isEmpty() ? "Aun no hay cortes cerrados."
                                                  : "Selecciona un corte para ver su comparativo.");
}

void CutoffView::onSelectionChanged() {
    const auto selected = m_cutoffTable->selectedItems();
    if (selected.isEmpty()) {
        m_exportSelectedButton->setEnabled(false);
        m_comparativeTable->setRowCount(0);
        return;
    }

    const int row = selected.first()->row();
    const qint64 cutoffId = m_cutoffTable->item(row, 0)->data(kRoleCutoffId).toLongLong();
    m_exportSelectedButton->setEnabled(true);
    reloadComparative(cutoffId);
}

void CutoffView::reloadComparative(qint64 cutoffId) {
    const int row = m_cutoffTable->currentRow();
    const QString period = row >= 0 ? m_cutoffTable->item(row, 0)->text() : QString();

    const auto comparative = m_cutoffEngine.compareWithPrevious(period);
    m_comparativeTable->setRowCount(0);

    if (comparative.isEmpty()) {
        m_comparativeHint->setText("Este es el primer corte registrado: no hay uno anterior con que comparar.");
        return;
    }

    m_comparativeHint->setText(QString("Comparando el corte %1 contra el corte anterior.").arg(period));
    const QLocale locale(QLocale::Spanish, QLocale::Mexico);

    for (const auto& item : comparative) {
        const int r = m_comparativeTable->rowCount();
        m_comparativeTable->insertRow(r);
        m_comparativeTable->setItem(r, 0, readOnlyItem(item.product.name));
        m_comparativeTable->setItem(r, 1, readOnlyItem(item.product.variant));
        m_comparativeTable->setItem(r, 2, readOnlyItem(locale.toString(item.previousQty, 'f', 0)));
        m_comparativeTable->setItem(r, 3, readOnlyItem(locale.toString(item.currentQty, 'f', 0)));

        const QString deltaText = (item.delta > 0 ? "+" : "") + locale.toString(item.delta, 'f', 0);
        auto* deltaItem = readOnlyItem(deltaText);
        if (item.delta < 0) {
            deltaItem->setForeground(QColor("#E48E93"));
        } else if (item.delta > 0) {
            deltaItem->setForeground(QColor("#8FD19E"));
        }
        m_comparativeTable->setItem(r, 4, deltaItem);
    }
}

void CutoffView::onCloseMonth() {
    bool ok = false;
    const QString defaultPeriod = QDate::currentDate().toString("yyyy-MM");
    const QString period =
        QInputDialog::getText(this, "Cerrar mes", "Periodo (AAAA-MM):", QLineEdit::Normal, defaultPeriod, &ok)
            .trimmed();
    if (!ok || period.isEmpty()) {
        return;
    }

    const QString note =
        QInputDialog::getText(this, "Cerrar mes", "Nota (opcional):", QLineEdit::Normal, {}, &ok).trimmed();

    const auto result = m_cutoffEngine.closeMonth(period, ok ? note : QString());
    if (!result.ok) {
        QMessageBox::warning(this, "Cerrar mes", result.error);
        return;
    }

    QMessageBox::information(this, "Cerrar mes", QString("Corte de %1 cerrado correctamente.").arg(period));
    reload();
    emit cutoffsChanged();
}

void CutoffView::onExportCurrentStock() {
    const auto report = m_reportEngine.currentStockReport();
    const QString defaultName = QString("stock_actual_%1.pdf").arg(QDate::currentDate().toString("yyyy-MM-dd"));
    const QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString path = QFileDialog::getSaveFileName(this, "Exportar stock actual", defaultDir + "/" + defaultName,
                                                        "Documento PDF (*.pdf)");
    if (path.isEmpty()) {
        return;
    }

    if (!exportReportToPdf(report, path)) {
        QMessageBox::warning(this, "Exportar PDF", "No se pudo generar el PDF.");
        return;
    }
    QMessageBox::information(this, "Exportar PDF", "Stock actual exportado correctamente.");
}

void CutoffView::onExportSelectedCutoff() {
    const auto selected = m_cutoffTable->selectedItems();
    if (selected.isEmpty()) {
        return;
    }
    const int row = selected.first()->row();
    const qint64 cutoffId = m_cutoffTable->item(row, 0)->data(kRoleCutoffId).toLongLong();
    const QString period = m_cutoffTable->item(row, 0)->text();

    const auto report = m_reportEngine.cutoffReport(cutoffId);
    const QString defaultName = QString("corte_%1.pdf").arg(period);
    const QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString path = QFileDialog::getSaveFileName(this, "Exportar corte", defaultDir + "/" + defaultName,
                                                        "Documento PDF (*.pdf)");
    if (path.isEmpty()) {
        return;
    }

    if (!exportReportToPdf(report, path)) {
        QMessageBox::warning(this, "Exportar PDF", "No se pudo generar el PDF.");
        return;
    }
    QMessageBox::information(this, "Exportar PDF", QString("Corte %1 exportado correctamente.").arg(period));
}

void CutoffView::onExportMovementReport() {
    const QDate fromDate = m_movementFromDate->date();
    const QDate toDate = m_movementToDate->date();
    if (fromDate > toDate) {
        QMessageBox::warning(this, "Reporte de movimientos", "La fecha \"Desde\" no puede ser posterior a \"Hasta\".");
        return;
    }

    const QDateTime from(fromDate, QTime(0, 0));
    const QDateTime to(toDate, QTime(23, 59, 59));
    const auto report = m_reportEngine.movementReport(from, to);

    if (report.rows.isEmpty()) {
        QMessageBox::information(this, "Reporte de movimientos",
                                  "No hay movimientos registrados en ese rango de fechas.");
        return;
    }

    const QString defaultName =
        QString("movimientos_%1_a_%2.pdf").arg(fromDate.toString("yyyy-MM-dd"), toDate.toString("yyyy-MM-dd"));
    const QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString path = QFileDialog::getSaveFileName(this, "Exportar reporte de movimientos",
                                                        defaultDir + "/" + defaultName, "Documento PDF (*.pdf)");
    if (path.isEmpty()) {
        return;
    }

    if (!exportMovementReportToPdf(report, path)) {
        QMessageBox::warning(this, "Exportar PDF", "No se pudo generar el PDF.");
        return;
    }
    QMessageBox::information(this, "Exportar PDF",
                              QString("%1 movimiento(s) exportado(s) correctamente.").arg(report.rows.size()));
}

} // namespace ui
