#include <QtTest/QtTest>
#include <memory>

#include "TestRuntimeGuard.h"
#include "data/CategoryRepository.h"
#include "data/CutoffRepository.h"
#include "data/Database.h"
#include "data/ProductRepository.h"
#include "domain/CutoffEngine.h"

using namespace data;
using namespace domain;

class CutoffEngineTest : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void cierraUnCorteYCongelaElStock();
    void noPermiteCerrarElMismoPeriodoDosVeces();
    void compareWithPreviousCalculaDeltas();
    void closeMonthAceptaNotaNula();

private:
    std::unique_ptr<Database> m_db;
    qint64 m_categoryId = -1;
};

void CutoffEngineTest::init() {
    m_db = std::make_unique<Database>(
        QStringLiteral("cutoff_test_conn_%1").arg(reinterpret_cast<quintptr>(this)));
    QVERIFY(m_db->open(":memory:"));

    CategoryRepository categories(m_db->handle());
    const auto allCategories = categories.all();
    QVERIFY(!allCategories.isEmpty());
    m_categoryId = allCategories.first().id;
}

void CutoffEngineTest::cleanup() {
    m_db.reset();
}

void CutoffEngineTest::cierraUnCorteYCongelaElStock() {
    ProductRepository products(m_db->handle());
    CutoffRepository cutoffs(m_db->handle());
    CutoffEngine engine(m_db->handle(), products, cutoffs);

    Product product;
    product.categoryId = m_categoryId;
    product.name = "Hojas blancas";
    product.unit = "Paquete";
    product.currentQty = 7;
    product.unitCost = 55.0;
    const qint64 id = products.insert(product);

    const auto result = engine.closeMonth("2026-09");
    QVERIFY(result.ok);

    const auto snapshot = cutoffs.snapshotForCutoff(result.cutoffId);
    QCOMPARE(snapshot.size(), 1);
    QCOMPARE(snapshot.first().productId, id);
    QCOMPARE(snapshot.first().quantity, 7.0);
    QCOMPARE(snapshot.first().value, 385.0);

    // cambiar el stock despues de cerrar el corte no debe afectar el
    // snapshot ya congelado
    products.updateQuantity(id, 99);
    const auto snapshotAfter = cutoffs.snapshotForCutoff(result.cutoffId);
    QCOMPARE(snapshotAfter.first().quantity, 7.0);
}

void CutoffEngineTest::noPermiteCerrarElMismoPeriodoDosVeces() {
    ProductRepository products(m_db->handle());
    CutoffRepository cutoffs(m_db->handle());
    CutoffEngine engine(m_db->handle(), products, cutoffs);

    QVERIFY(engine.closeMonth("2026-09").ok);
    const auto second = engine.closeMonth("2026-09");
    QVERIFY(!second.ok);
}

void CutoffEngineTest::compareWithPreviousCalculaDeltas() {
    ProductRepository products(m_db->handle());
    CutoffRepository cutoffs(m_db->handle());
    CutoffEngine engine(m_db->handle(), products, cutoffs);

    Product product;
    product.categoryId = m_categoryId;
    product.name = "Folder beige";
    product.unit = "Pza";
    product.currentQty = 60;
    const qint64 id = products.insert(product);

    QVERIFY(engine.closeMonth("2026-08").ok);

    products.updateQuantity(id, 45);
    QVERIFY(engine.closeMonth("2026-09").ok);

    const auto comparative = engine.compareWithPrevious("2026-09");
    QCOMPARE(comparative.size(), 1);
    QCOMPARE(comparative.first().previousQty, 60.0);
    QCOMPARE(comparative.first().currentQty, 45.0);
    QCOMPARE(comparative.first().delta, -15.0);
}

void CutoffEngineTest::closeMonthAceptaNotaNula() {
    // Reproduce el bug real encontrado en la UI: un QLineEdit/QInputDialog
    // sin tocar puede devolver una QString nula (distinta de ""), y
    // bindear eso en SQLite viola la columna NOT NULL de `note`. El motor
    // debe funcionar sin importar lo que la UI le pase.
    ProductRepository products(m_db->handle());
    CutoffRepository cutoffs(m_db->handle());
    CutoffEngine engine(m_db->handle(), products, cutoffs);

    const auto result = engine.closeMonth("2026-09", QString());
    QVERIFY(result.ok);
}

QTEST_MAIN(CutoffEngineTest)
#include "CutoffEngineTest.moc"
