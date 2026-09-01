#include "app/ThemeManager.h"

#include <QApplication>
#include <QFile>
#include <QTextStream>

namespace app {

void ThemeManager::apply(QApplication& application) {
    QFile file(":/theme.qss");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QTextStream stream(&file);
    application.setStyleSheet(stream.readAll());
}

} // namespace app
