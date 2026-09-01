#pragma once

class QApplication;

namespace app {

// Aplica el tema visual propio de Inventario (grafito + ambar) a toda la
// aplicacion. Toda la apariencia vive en resources/theme.qss; esta clase
// solo se encarga de cargarlo y aplicarlo.
class ThemeManager {
public:
    static void apply(QApplication& application);
};

} // namespace app
