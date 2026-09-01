#pragma once

class QStackedWidget;

namespace app {

// Transicion ligera (fundido cruzado) al cambiar de pagina en un
// QStackedWidget. Pensada para ser barata en CPU: solo anima opacidad,
// sin efectos de sombra/blur pesados.
void animatedSetCurrentIndex(QStackedWidget* stack, int index, int durationMs = 180);

} // namespace app
