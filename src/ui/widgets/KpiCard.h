#pragma once

#include <QWidget>
#include <functional>

class QLabel;
class QVariantAnimation;

namespace ui {

// Tarjeta de indicador (KPI) con un contador animado: al llamar
// setValue(), el numero mostrado interpola suavemente desde el valor
// anterior hasta el nuevo en vez de saltar de golpe.
class KpiCard : public QWidget {
    Q_OBJECT

public:
    explicit KpiCard(const QString& title, QWidget* parent = nullptr);

    using Formatter = std::function<QString(double)>;
    void setFormatter(Formatter formatter);
    void setValue(double value);

private:
    QLabel* m_valueLabel = nullptr;
    QVariantAnimation* m_animation = nullptr;
    Formatter m_formatter;
    double m_currentValue = 0.0;
};

} // namespace ui
