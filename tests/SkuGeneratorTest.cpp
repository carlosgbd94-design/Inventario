#include <QtTest/QtTest>

#include "domain/SkuGenerator.h"

using namespace domain;

class SkuGeneratorTest : public QObject {
    Q_OBJECT

private slots:
    void combinaCategoriaNombreYVariante();
    void ignoraAcentosYCaracteresRaros();
    void sinVariante();
    void evitaChoquesConSufijoNumerico();
};

void SkuGeneratorTest::combinaCategoriaNombreYVariante() {
    const QString sku = generateSku("Uniformes", "Chaleco azul marino", "XG", {});
    QCOMPARE(sku, QString("UNI-CHAL-XG"));
}

void SkuGeneratorTest::ignoraAcentosYCaracteresRaros() {
    const QString sku = generateSku("Papelería", "Engrapadora N°2", "", {});
    QCOMPARE(sku, QString("PAP-ENGR"));
}

void SkuGeneratorTest::sinVariante() {
    const QString sku = generateSku("Papeleria", "Clips", "", {});
    QCOMPARE(sku, QString("PAP-CLIP"));
}

void SkuGeneratorTest::evitaChoquesConSufijoNumerico() {
    const QSet<QString> existing = {"UNI-CHAL-XG", "UNI-CHAL-XG-2"};
    const QString sku = generateSku("Uniformes", "Chaleco azul marino", "XG", existing);
    QCOMPARE(sku, QString("UNI-CHAL-XG-3"));
}

QTEST_MAIN(SkuGeneratorTest)
#include "SkuGeneratorTest.moc"
