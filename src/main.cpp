#include <QApplication>
#include <QIcon>

#include "app/ThemeManager.h"
#include "ui/MainWindow.h"
#include "version.h"

int main(int argc, char* argv[]) {
    QApplication application(argc, argv);
    QApplication::setApplicationName(APP_NAME);
    QApplication::setApplicationVersion(APP_VERSION_STRING);
    QApplication::setWindowIcon(QIcon(":/logo.svg"));

    app::ThemeManager::apply(application);

    ui::MainWindow window;
    window.show();

    return QApplication::exec();
}
