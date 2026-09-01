// Herramienta de un solo uso para rasterizar assets/logo.svg a PNG en
// varias resoluciones (para armar el .ico del instalador). No forma
// parte del build normal del programa.
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QSvgRenderer>

int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);
    if (argc < 4) {
        return 1;
    }

    QSvgRenderer renderer(QString::fromLocal8Bit(argv[1]));
    if (!renderer.isValid()) {
        return 2;
    }

    const int size = QString::fromLocal8Bit(argv[2]).toInt();
    QImage image(size, size, QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    renderer.render(&painter, QRectF(0, 0, size, size));
    painter.end();

    return image.save(QString::fromLocal8Bit(argv[3])) ? 0 : 3;
}
