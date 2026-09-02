#include <QApplication>
#include <QIcon>
#include <QMessageBox>

#include "app/ThemeManager.h"
#include "data/Database.h"
#include "ui/MainWindow.h"
#include "version.h"

int main(int argc, char* argv[]) {
    QApplication application(argc, argv);
    QApplication::setApplicationName(APP_NAME);
    QApplication::setApplicationVersion(APP_VERSION_STRING);
    QApplication::setWindowIcon(QIcon(":/logo.png"));

    app::ThemeManager::apply(application);

    data::Database database("main");
    if (!database.open(data::Database::defaultDatabasePath())) {
        QMessageBox::critical(nullptr, APP_NAME, "No se pudo abrir la base de datos local.");
        return 1;
    }

    ui::MainWindow window(database.handle());
    window.show();

    return QApplication::exec();
}
