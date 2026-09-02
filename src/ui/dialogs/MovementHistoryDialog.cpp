#include "ui/dialogs/MovementHistoryDialog.h"

#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QFile>
#include <QHeaderView>
#include <QLabel>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QUrl>
#include <QVBoxLayout>

namespace ui {

namespace {
QTableWidgetItem* readOnlyItem(const QString& text) {
    auto* item = new QTableWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    return item;
}

QString typeLabel(data::MovementType type) {
    switch (type) {
        case data::MovementType::Entrada: return "Entrada";
        case data::MovementType::Salida: return "Salida";
        case data::MovementType::Ajuste: return "Ajuste";
    }
    return "Entrada";
}
} // namespace

MovementHistoryDialog::MovementHistoryDialog(const data::Product& product,
                                               const QVector<data::StockMovement>& movements, QWidget* parent)
    : QDialog(parent), m_movements(movements) {
    setWindowTitle(QString("Historial - %1 %2").arg(product.name, product.variant).trimmed());
    setModal(true);
    resize(560, 420);

    auto* rootLayout = new QVBoxLayout(this);

    if (m_movements.isEmpty()) {
        auto* empty = new QLabel("Este producto no tiene movimientos registrados todavia.", this);
        empty->setObjectName("SubtitleLabel");
        rootLayout->addWidget(empty);
    }

    m_table = new QTableWidget(m_movements.size(), 5, this);
    m_table->setHorizontalHeaderLabels({"Fecha", "Tipo", "Cantidad", "Nota", "Adjunto"});
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);

    const QLocale locale(QLocale::Spanish, QLocale::Mexico);
    for (int row = 0; row < m_movements.size(); ++row) {
        const data::StockMovement& movement = m_movements.at(row);
        m_table->setItem(row, 0, readOnlyItem(movement.date.toString("dd/MM/yyyy hh:mm")));
        m_table->setItem(row, 1, readOnlyItem(typeLabel(movement.type)));

        const QString qtyText = (movement.quantity > 0 ? "+" : "") + locale.toString(movement.quantity, 'f', 2);
        m_table->setItem(row, 2, readOnlyItem(qtyText));
        m_table->setItem(row, 3, readOnlyItem(movement.note));

        if (movement.attachmentPath.isEmpty()) {
            m_table->setItem(row, 4, readOnlyItem("-"));
        } else {
            auto* openButton = new QPushButton("Abrir", m_table);
            openButton->setCursor(Qt::PointingHandCursor);
            // Sin esto, el alto de fila por defecto de QTableWidget corta
            // el boton (su padding normal es mas alto que la fila) y solo
            // se ve el fondo redondeado, sin el texto.
            openButton->setStyleSheet("padding: 2px 12px;");
            connect(openButton, &QPushButton::clicked, this, [this, row]() { onOpenAttachment(row); });
            m_table->setCellWidget(row, 4, openButton);
        }
    }
    m_table->resizeRowsToContents();

    rootLayout->addWidget(m_table, 1);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons->button(QDialogButtonBox::Close), &QPushButton::clicked, this, &QDialog::accept);
    rootLayout->addWidget(buttons);
}

void MovementHistoryDialog::onOpenAttachment(int row) {
    if (row < 0 || row >= m_movements.size()) {
        return;
    }
    const QString path = m_movements.at(row).attachmentPath;
    if (path.isEmpty() || !QFile::exists(path)) {
        QMessageBox::warning(this, "Adjunto", "No se encontro el archivo adjunto en el disco.");
        return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

} // namespace ui
