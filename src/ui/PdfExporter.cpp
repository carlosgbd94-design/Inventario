#include "ui/PdfExporter.h"

#include <QDateTime>
#include <QFontMetrics>
#include <QLocale>
#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QPixmap>
#include <cmath>
#include <functional>

#include "domain/ReportEngine.h"

namespace ui {

namespace {

using RowToCells = std::function<QStringList(int)>;

QVector<int> columnEdges(int pageWidth, const QVector<double>& fractions) {
    QVector<int> edges(fractions.size() + 1);
    edges[0] = 0;
    for (int i = 0; i < fractions.size(); ++i) {
        edges[i + 1] = edges[i] + static_cast<int>(pageWidth * fractions[i]);
    }
    edges[edges.size() - 1] = pageWidth;
    return edges;
}

void drawRowCells(QPainter& painter, const QVector<int>& colX, int y, int rowHeight, const QStringList& cells,
                   const QVector<bool>& rightAlignColumns) {
    const QFontMetrics metrics(painter.font());
    for (int c = 0; c < cells.size(); ++c) {
        const Qt::Alignment align =
            rightAlignColumns[c] ? (Qt::AlignRight | Qt::AlignVCenter) : (Qt::AlignLeft | Qt::AlignVCenter);
        const QRect cellRect(colX[c] + 6, y, colX[c + 1] - colX[c] - 10, rowHeight);
        const QString elided = metrics.elidedText(cells[c], Qt::ElideRight, cellRect.width());
        painter.drawText(cellRect, align, elided);
    }
}

// Arma el PDF paginado (logo, titulo, tabla con encabezado repetido en
// cada pagina) que comparten el reporte de stock y el de movimientos;
// solo cambian los encabezados de columna y de donde sale cada fila.
bool renderPaginatedReport(const QString& filePath, const QString& title, const QString& subtitle,
                            const QStringList& headers, const QVector<double>& columnFractions,
                            const QVector<bool>& rightAlignColumns, int rowCount, const RowToCells& rowToCells,
                            const QString& footerText) {
    QPdfWriter writer(filePath);
    writer.setPageSize(QPageSize(QPageSize::Letter));
    writer.setPageMargins(QMarginsF(16, 16, 16, 16), QPageLayout::Millimeter);
    writer.setResolution(150);

    QPainter painter;
    if (!painter.begin(&writer)) {
        return false;
    }

    const int pageWidth = painter.viewport().width();
    const int pageHeight = painter.viewport().height();
    const QVector<int> colX = columnEdges(pageWidth, columnFractions);

    const QColor textPrimary(25, 25, 25);
    const QColor textSecondary(100, 100, 100);
    const QColor headerBg(233, 233, 233);
    const QColor gridLine(210, 210, 210);

    const int rowHeight = 26;
    const int headerRowHeight = 30;
    const int bottomReserved = 70;

    int y = 0;

    auto drawPageHeader = [&](bool firstPage) {
        y = 0;
        if (firstPage) {
            const QPixmap logo(QStringLiteral(":/logo.png"));
            if (!logo.isNull()) {
                painter.drawPixmap(QRectF(0, y, 46, 46), logo, logo.rect());
            }
            painter.setFont(QFont("Segoe UI", 16, QFont::Bold));
            painter.setPen(textPrimary);
            painter.drawText(QRect(58, y, pageWidth - 58, 40), Qt::AlignVCenter | Qt::AlignLeft, "Inventario");
            y += 56;

            painter.setFont(QFont("Segoe UI", 13, QFont::Bold));
            painter.drawText(QRect(0, y, pageWidth, 26), Qt::AlignLeft | Qt::AlignVCenter, title);
            y += 28;

            painter.setFont(QFont("Segoe UI", 10));
            painter.setPen(textSecondary);
            painter.drawText(QRect(0, y, pageWidth, 20), Qt::AlignLeft | Qt::AlignVCenter, subtitle);
            y += 20;

            painter.drawText(QRect(0, y, pageWidth, 18), Qt::AlignLeft | Qt::AlignVCenter,
                              "Generado el " + QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm"));
            y += 26;
        } else {
            painter.setFont(QFont("Segoe UI", 10, QFont::Bold));
            painter.setPen(textSecondary);
            painter.drawText(QRect(0, y, pageWidth, 22), Qt::AlignLeft | Qt::AlignVCenter, title + " (continuacion)");
            y += 28;
        }

        painter.fillRect(QRect(0, y, pageWidth, headerRowHeight), headerBg);
        painter.setFont(QFont("Segoe UI", 9, QFont::DemiBold));
        painter.setPen(textPrimary);
        drawRowCells(painter, colX, y, headerRowHeight, headers, rightAlignColumns);
        y += headerRowHeight;
        painter.setPen(gridLine);
        painter.drawLine(0, y, pageWidth, y);
    };

    drawPageHeader(true);

    for (int i = 0; i < rowCount; ++i) {
        if (y + rowHeight > pageHeight - bottomReserved) {
            writer.newPage();
            drawPageHeader(false);
        }

        painter.setFont(QFont("Segoe UI", 9));
        painter.setPen(textPrimary);
        drawRowCells(painter, colX, y, rowHeight, rowToCells(i), rightAlignColumns);
        y += rowHeight;
        painter.setPen(gridLine);
        painter.drawLine(0, y, pageWidth, y);
    }

    if (!footerText.isEmpty()) {
        if (y + 40 > pageHeight - 10) {
            writer.newPage();
            drawPageHeader(false);
        }
        y += 16;

        painter.setFont(QFont("Segoe UI", 11, QFont::Bold));
        painter.setPen(textPrimary);
        painter.drawText(QRect(0, y, pageWidth, 30), Qt::AlignRight | Qt::AlignVCenter, footerText);
    }

    painter.end();
    return true;
}

QString movementTypeLabel(data::MovementType type) {
    switch (type) {
        case data::MovementType::Entrada:
            return "Entrada";
        case data::MovementType::Salida:
            return "Salida";
        case data::MovementType::Ajuste:
            return "Ajuste";
    }
    return "Entrada";
}

} // namespace

bool exportReportToPdf(const domain::ReportData& data, const QString& filePath) {
    static const QVector<double> kColumnFractions = {0.12, 0.24, 0.14, 0.10, 0.12, 0.14, 0.14};
    static const QStringList kHeaders = {"Codigo", "Producto", "Variante", "Unidad", "Cantidad",
                                          "Costo unitario", "Valor"};
    static const QVector<bool> kRightAlign = {false, false, false, false, true, true, true};
    const QLocale locale(QLocale::Spanish, QLocale::Mexico);

    const auto rowToCells = [&](int i) {
        const domain::ReportRow& row = data.rows[i];
        return QStringList{
            row.sku,
            row.productName,
            row.variant,
            row.unit,
            locale.toString(row.quantity, 'f', row.quantity == std::floor(row.quantity) ? 0 : 2),
            locale.toCurrencyString(row.unitCost, "$"),
            locale.toCurrencyString(row.value, "$"),
        };
    };

    return renderPaginatedReport(filePath, data.title, data.subtitle, kHeaders, kColumnFractions, kRightAlign,
                                  data.rows.size(), rowToCells,
                                  "Valor total: " + locale.toCurrencyString(data.totalValue, "$"));
}

bool exportMovementReportToPdf(const domain::MovementReportData& data, const QString& filePath) {
    static const QVector<double> kColumnFractions = {0.13, 0.12, 0.22, 0.13, 0.11, 0.10, 0.19};
    static const QStringList kHeaders = {"Fecha", "Codigo", "Producto", "Variante", "Tipo", "Cantidad", "Nota"};
    static const QVector<bool> kRightAlign = {false, false, false, false, false, true, false};
    const QLocale locale(QLocale::Spanish, QLocale::Mexico);

    const auto rowToCells = [&](int i) {
        const domain::MovementReportRow& row = data.rows[i];
        const QString qtyText =
            (row.quantity > 0 ? "+" : "") + locale.toString(row.quantity, 'f', row.quantity == std::floor(row.quantity) ? 0 : 2);
        return QStringList{
            row.date.toString("dd/MM/yyyy hh:mm"),
            row.sku,
            row.productName,
            row.variant,
            movementTypeLabel(row.type),
            qtyText,
            row.note,
        };
    };

    return renderPaginatedReport(filePath, data.title, data.subtitle, kHeaders, kColumnFractions, kRightAlign,
                                  data.rows.size(), rowToCells, QString());
}

} // namespace ui
