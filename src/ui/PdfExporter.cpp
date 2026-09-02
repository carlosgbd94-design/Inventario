#include "ui/PdfExporter.h"

#include <QDateTime>
#include <QFontMetrics>
#include <QLocale>
#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QSvgRenderer>
#include <cmath>

#include "domain/ReportEngine.h"

namespace ui {

namespace {

const QVector<double> kColumnFractions = {0.12, 0.24, 0.14, 0.10, 0.12, 0.14, 0.14};
const QStringList kHeaders = {"Codigo", "Producto", "Variante", "Unidad", "Cantidad", "Costo unitario", "Valor"};
constexpr int kFirstNumericColumn = 4;

QVector<int> columnEdges(int pageWidth) {
    QVector<int> edges(kColumnFractions.size() + 1);
    edges[0] = 0;
    for (int i = 0; i < kColumnFractions.size(); ++i) {
        edges[i + 1] = edges[i] + static_cast<int>(pageWidth * kColumnFractions[i]);
    }
    edges[edges.size() - 1] = pageWidth;
    return edges;
}

void drawRowCells(QPainter& painter, const QVector<int>& colX, int y, int rowHeight, const QStringList& cells) {
    const QFontMetrics metrics(painter.font());
    for (int c = 0; c < cells.size(); ++c) {
        const Qt::Alignment align =
            (c >= kFirstNumericColumn) ? (Qt::AlignRight | Qt::AlignVCenter) : (Qt::AlignLeft | Qt::AlignVCenter);
        const QRect cellRect(colX[c] + 6, y, colX[c + 1] - colX[c] - 10, rowHeight);
        const QString elided = metrics.elidedText(cells[c], Qt::ElideRight, cellRect.width());
        painter.drawText(cellRect, align, elided);
    }
}

} // namespace

bool exportReportToPdf(const domain::ReportData& data, const QString& filePath) {
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
    const QVector<int> colX = columnEdges(pageWidth);

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
            QSvgRenderer svg(QStringLiteral(":/logo.svg"));
            if (svg.isValid()) {
                svg.render(&painter, QRectF(0, y, 46, 46));
            }
            painter.setFont(QFont("Segoe UI", 16, QFont::Bold));
            painter.setPen(textPrimary);
            painter.drawText(QRect(58, y, pageWidth - 58, 40), Qt::AlignVCenter | Qt::AlignLeft, "Inventario");
            y += 56;

            painter.setFont(QFont("Segoe UI", 13, QFont::Bold));
            painter.drawText(QRect(0, y, pageWidth, 26), Qt::AlignLeft | Qt::AlignVCenter, data.title);
            y += 28;

            painter.setFont(QFont("Segoe UI", 10));
            painter.setPen(textSecondary);
            painter.drawText(QRect(0, y, pageWidth, 20), Qt::AlignLeft | Qt::AlignVCenter, data.subtitle);
            y += 20;

            painter.drawText(QRect(0, y, pageWidth, 18), Qt::AlignLeft | Qt::AlignVCenter,
                              "Generado el " + QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm"));
            y += 26;
        } else {
            painter.setFont(QFont("Segoe UI", 10, QFont::Bold));
            painter.setPen(textSecondary);
            painter.drawText(QRect(0, y, pageWidth, 22), Qt::AlignLeft | Qt::AlignVCenter,
                              data.title + " (continuacion)");
            y += 28;
        }

        painter.fillRect(QRect(0, y, pageWidth, headerRowHeight), headerBg);
        painter.setFont(QFont("Segoe UI", 9, QFont::DemiBold));
        painter.setPen(textPrimary);
        drawRowCells(painter, colX, y, headerRowHeight, kHeaders);
        y += headerRowHeight;
        painter.setPen(gridLine);
        painter.drawLine(0, y, pageWidth, y);
    };

    const QLocale locale(QLocale::Spanish, QLocale::Mexico);

    drawPageHeader(true);

    for (const domain::ReportRow& row : data.rows) {
        if (y + rowHeight > pageHeight - bottomReserved) {
            writer.newPage();
            drawPageHeader(false);
        }

        painter.setFont(QFont("Segoe UI", 9));
        painter.setPen(textPrimary);
        const QStringList cells = {
            row.sku,
            row.productName,
            row.variant,
            row.unit,
            locale.toString(row.quantity, 'f', row.quantity == std::floor(row.quantity) ? 0 : 2),
            locale.toCurrencyString(row.unitCost, "$"),
            locale.toCurrencyString(row.value, "$"),
        };
        drawRowCells(painter, colX, y, rowHeight, cells);
        y += rowHeight;
        painter.setPen(gridLine);
        painter.drawLine(0, y, pageWidth, y);
    }

    if (y + 40 > pageHeight - 10) {
        writer.newPage();
        drawPageHeader(false);
    }
    y += 16;

    painter.setFont(QFont("Segoe UI", 11, QFont::Bold));
    painter.setPen(textPrimary);
    painter.drawText(QRect(0, y, pageWidth, 30), Qt::AlignRight | Qt::AlignVCenter,
                      "Valor total: " + locale.toCurrencyString(data.totalValue, "$"));

    painter.end();
    return true;
}

} // namespace ui
