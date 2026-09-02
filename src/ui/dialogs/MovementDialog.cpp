#include "ui/dialogs/MovementDialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMimeData>
#include <QPushButton>
#include <QStyle>
#include <QUrl>
#include <QVBoxLayout>

namespace ui {

namespace {
// Mismos formatos que acepta el selector de archivos del boton
// "Adjuntar...", para que arrastrar-y-soltar no permita colar
// cualquier cosa.
bool isAcceptedAttachment(const QString& filePath) {
    static const QStringList kExtensions = {"pdf", "png", "jpg", "jpeg"};
    return kExtensions.contains(QFileInfo(filePath).suffix().toLower());
}
} // namespace

MovementDialog::MovementDialog(const data::Product& product, QWidget* parent)
    : QDialog(parent), m_product(product) {
    setWindowTitle(QString("Movimiento - %1 %2").arg(product.name, product.variant).trimmed());
    setModal(true);
    setMinimumWidth(340);
    setAcceptDrops(true);

    auto* rootLayout = new QVBoxLayout(this);

    auto* info = new QLabel(QString("Existencia actual: %1 %2").arg(product.currentQty).arg(product.unit), this);
    info->setObjectName("SubtitleLabel");
    rootLayout->addWidget(info);

    auto* form = new QFormLayout();
    form->setSpacing(10);

    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItem("Entrada", static_cast<int>(data::MovementType::Entrada));
    m_typeCombo->addItem("Salida", static_cast<int>(data::MovementType::Salida));
    m_typeCombo->addItem("Ajuste por recuento fisico", static_cast<int>(data::MovementType::Ajuste));
    form->addRow("Tipo", m_typeCombo);

    m_qtyLabel = new QLabel("Cantidad", this);
    m_qtySpin = new QDoubleSpinBox(this);
    m_qtySpin->setRange(0.0, 999999.0);
    m_qtySpin->setDecimals(2);
    m_qtySpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    form->addRow(m_qtyLabel, m_qtySpin);

    m_noteEdit = new QLineEdit(this);
    m_noteEdit->setPlaceholderText("Opcional");
    form->addRow("Nota", m_noteEdit);

    auto* attachmentLayout = new QHBoxLayout();
    m_attachmentLabel = new QLabel("Sin archivo adjunto (o arrastralo aqui)", this);
    m_attachmentLabel->setObjectName("VersionLabel");
    m_attachmentLabel->setWordWrap(true);
    attachmentLayout->addWidget(m_attachmentLabel, 1);

    auto* attachButton = new QPushButton("Adjuntar...", this);
    attachButton->setCursor(Qt::PointingHandCursor);
    attachmentLayout->addWidget(attachButton);

    m_clearAttachmentButton = new QPushButton("Quitar", this);
    m_clearAttachmentButton->setCursor(Qt::PointingHandCursor);
    m_clearAttachmentButton->setVisible(false);
    attachmentLayout->addWidget(m_clearAttachmentButton);

    form->addRow("Factura / ticket", attachmentLayout);

    rootLayout->addLayout(form);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    QPushButton* okButton = buttons->button(QDialogButtonBox::Ok);
    okButton->setObjectName("PrimaryButton");
    okButton->style()->unpolish(okButton);
    okButton->style()->polish(okButton);
    rootLayout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MovementDialog::onTypeChanged);

    connect(attachButton, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getOpenFileName(this, "Adjuntar factura o ticket", {},
                                                            "Documentos e imagenes (*.pdf *.png *.jpg *.jpeg)");
        if (!path.isEmpty()) {
            setAttachment(path);
        }
    });

    connect(m_clearAttachmentButton, &QPushButton::clicked, this, [this]() { setAttachment({}); });

    onTypeChanged(m_typeCombo->currentIndex());
}

void MovementDialog::setAttachment(const QString& path) {
    m_attachmentPath = path;
    m_attachmentLabel->setText(path.isEmpty() ? "Sin archivo adjunto (o arrastralo aqui)" : QFileInfo(path).fileName());
    m_clearAttachmentButton->setVisible(!path.isEmpty());
}

void MovementDialog::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        const QList<QUrl> urls = event->mimeData()->urls();
        if (urls.size() == 1 && urls.first().isLocalFile() && isAcceptedAttachment(urls.first().toLocalFile())) {
            event->acceptProposedAction();
        }
    }
}

void MovementDialog::dropEvent(QDropEvent* event) {
    const QList<QUrl> urls = event->mimeData()->urls();
    if (urls.size() == 1 && urls.first().isLocalFile()) {
        setAttachment(urls.first().toLocalFile());
        event->acceptProposedAction();
    }
}

void MovementDialog::onTypeChanged(int /*index*/) {
    const auto type = selectedType();
    if (type == data::MovementType::Ajuste) {
        m_qtyLabel->setText("Cantidad real contada");
        m_qtySpin->setValue(m_product.currentQty);
    } else {
        m_qtyLabel->setText("Cantidad");
        m_qtySpin->setValue(0.0);
    }
}

data::MovementType MovementDialog::selectedType() const {
    return static_cast<data::MovementType>(m_typeCombo->currentData().toInt());
}

double MovementDialog::quantity() const {
    return m_qtySpin->value();
}

QString MovementDialog::note() const {
    return m_noteEdit->text().trimmed();
}

QString MovementDialog::selectedAttachmentPath() const {
    return m_attachmentPath;
}

} // namespace ui
