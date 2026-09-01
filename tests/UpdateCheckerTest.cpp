#include <QtTest/QtTest>

#include "update/UpdateChecker.h"

using namespace update;

class UpdateCheckerTest : public QObject {
    Q_OBJECT

private slots:
    void versionesIguales();
    void versionMasNuevaEsMayor();
    void versionMasViejaEsMenor();
    void toleraPrefijoV();
    void componentesFaltantesCuentanComoCero();
};

void UpdateCheckerTest::versionesIguales() {
    QCOMPARE(compareVersions("1.2.3", "1.2.3"), 0);
}

void UpdateCheckerTest::versionMasNuevaEsMayor() {
    QVERIFY(compareVersions("1.3.0", "1.2.9") > 0);
    QVERIFY(compareVersions("2.0.0", "1.9.9") > 0);
}

void UpdateCheckerTest::versionMasViejaEsMenor() {
    QVERIFY(compareVersions("1.2.0", "1.2.5") < 0);
}

void UpdateCheckerTest::toleraPrefijoV() {
    QCOMPARE(compareVersions("v1.0.0", "1.0.0"), 0);
    QVERIFY(compareVersions("v1.1.0", "v1.0.0") > 0);
}

void UpdateCheckerTest::componentesFaltantesCuentanComoCero() {
    QCOMPARE(compareVersions("1.0", "1.0.0"), 0);
    QVERIFY(compareVersions("1.1", "1.0.9") > 0);
}

QTEST_APPLESS_MAIN(UpdateCheckerTest)
#include "UpdateCheckerTest.moc"
