#include "ui/widgets/KpiCard.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QVariantAnimation>

namespace ui {

KpiCard::KpiCard(const QString& title, QWidget* parent) : QWidget(parent) {
    setObjectName("SurfaceCard");
    setAttribute(Qt::WA_StyledBackground, true);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 18, 20, 18);
    layout->setSpacing(6);

    auto* titleLabel = new QLabel(title, this);
    titleLabel->setObjectName("SubtitleLabel");
    layout->addWidget(titleLabel);

    m_valueLabel = new QLabel("0", this);
    m_valueLabel->setObjectName("KpiValueLabel");
    layout->addWidget(m_valueLabel);

    m_formatter = [](double value) { return QString::number(value, 'f', 0); };

    m_animation = new QVariantAnimation(this);
    m_animation->setDuration(500);
    m_animation->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_animation, &QVariantAnimation::valueChanged, this, [this](const QVariant& v) {
        m_valueLabel->setText(m_formatter(v.toDouble()));
    });
}

void KpiCard::setFormatter(Formatter formatter) {
    m_formatter = std::move(formatter);
}

void KpiCard::setValue(double value) {
    m_animation->stop();
    m_animation->setStartValue(m_currentValue);
    m_animation->setEndValue(value);
    m_currentValue = value;
    m_animation->start();
}

} // namespace ui
