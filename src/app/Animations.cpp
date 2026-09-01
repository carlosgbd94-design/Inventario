#include "app/Animations.h"

#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QStackedWidget>

namespace app {

void animatedSetCurrentIndex(QStackedWidget* stack, int index, int durationMs) {
    if (!stack || index == stack->currentIndex()) {
        return;
    }

    QWidget* incoming = stack->widget(index);
    if (!incoming) {
        return;
    }

    stack->setCurrentIndex(index);

    auto* effect = new QGraphicsOpacityEffect(incoming);
    incoming->setGraphicsEffect(effect);

    auto* fadeIn = new QPropertyAnimation(effect, "opacity", stack);
    fadeIn->setDuration(durationMs);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->setEasingCurve(QEasingCurve::OutCubic);

    // Nunca dejar un QGraphicsOpacityEffect pegado despues de terminar: un
    // widget con efecto se queda en la ruta de composicion lenta, y si
    // dentro de el aparece OTRO widget con su propio efecto (p.ej. al
    // cambiar de vista dentro de esta misma pagina) los efectos anidados
    // pueden dejar el contenido interno sin pintarse.
    QObject::connect(fadeIn, &QPropertyAnimation::finished, incoming, [incoming]() {
        incoming->setGraphicsEffect(nullptr);
    });

    fadeIn->start(QAbstractAnimation::DeleteWhenStopped);
}

} // namespace app
