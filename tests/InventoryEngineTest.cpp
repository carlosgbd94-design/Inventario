#include <QtTest/QtTest>
#include <memory>

#include "TestRuntimeGuard.h"
#include "data/CategoryRepository.h"
#include "data/Database.h"
#include "data/MovementRepository.h"
#include "data/ProductRepository.h"
#include "domain/InventoryEngine.h"

using namespace data;
using namespace domain;

class InventoryEngineTest : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void entradaAumentaStock();
    void salidaNoPuedeDejarNegativo();
    void ajusteEstableceCantidadAbsoluta();
    void sugerenciasDeReorden();

private:
    std::unique_ptr<Database> m_db;
    qint64 m_categoryId = -1;
};

void InventoryEngineTest::init() {
    m_db = std::make_unique<Database>(
        QStringLiteral("inventory_test_conn_%1").arg(reinterpret_cast<quintptr>(this)));
    QVERIFY(m_db->open(":memory:"));

    CategoryRepository categories(m_db->handle());
    const auto allCategories = categories.all();
    QVERIFY(!allCategories.isEmpty());
    m_categoryId = allCategories.first().id;
}

void InventoryEngineTest::cleanup() {
    m_db.reset();
}

void InventoryEngineTest::entradaAumentaStock() {
    ProductRepository products(m_db->handle());
    MovementRepository movements(m_db->handle());
    InventoryEngine engine(m_db->handle(), products, movements);

    Product product;
    product.categoryId = m_categoryId;
    product.name = "Playera polo roja";
    product.variant = "Chica";
    product.unit = "Pza";
    product.currentQty = 5;
    const qint64 id = products.insert(product);
    QVERIFY(id > 0);

    InventoryEngine::MovementInput input;
    input.productId = id;
    input.type = MovementType::Entrada;
    input.quantity = 10;
    input.note = "compra";
    const auto result = engine.registerMovement(input);
    QVERIFY(result.ok);
    QCOMPARE(result.newQuantity, 15.0);

    const auto reloaded = products.byId(id);
    QVERIFY(reloaded.has_value());
    QCOMPARE(reloaded->currentQty, 15.0);

    const auto history = movements.byProduct(id);
    QCOMPARE(history.size(), 1);
    QCOMPARE(history.first().quantity, 10.0);
}

void InventoryEngineTest::salidaNoPuedeDejarNegativo() {
    ProductRepository products(m_db->handle());
    MovementRepository movements(m_db->handle());
    InventoryEngine engine(m_db->handle(), products, movements);

    Product product;
    product.categoryId = m_categoryId;
    product.name = "Folder rojo";
    product.unit = "Pza";
    product.currentQty = 3;
    const qint64 id = products.insert(product);

    InventoryEngine::MovementInput input;
    input.productId = id;
    input.type = MovementType::Salida;
    input.quantity = 5;
    const auto result = engine.registerMovement(input);
    QVERIFY(!result.ok);

    const auto reloaded = products.byId(id);
    QVERIFY(reloaded.has_value());
    QCOMPARE(reloaded->currentQty, 3.0);
}

void InventoryEngineTest::ajusteEstableceCantidadAbsoluta() {
    ProductRepository products(m_db->handle());
    MovementRepository movements(m_db->handle());
    InventoryEngine engine(m_db->handle(), products, movements);

    Product product;
    product.categoryId = m_categoryId;
    product.name = "Sobre amarillo";
    product.unit = "Pza";
    product.currentQty = 20;
    const qint64 id = products.insert(product);

    InventoryEngine::MovementInput input;
    input.productId = id;
    input.type = MovementType::Ajuste;
    input.quantity = 7;
    input.note = "recuento fisico";
    const auto result = engine.registerMovement(input);
    QVERIFY(result.ok);
    QCOMPARE(result.newQuantity, 7.0);

    const auto history = movements.byProduct(id);
    QCOMPARE(history.first().quantity, -13.0);
}

void InventoryEngineTest::sugerenciasDeReorden() {
    ProductRepository products(m_db->handle());

    Product low;
    low.categoryId = m_categoryId;
    low.name = "Playera polo negra";
    low.unit = "Pza";
    low.currentQty = 2;
    low.minStock = 5;
    products.insert(low);

    Product ok;
    ok.categoryId = m_categoryId;
    ok.name = "Playera polo azul";
    ok.unit = "Pza";
    ok.currentQty = 20;
    ok.minStock = 5;
    products.insert(ok);

    MovementRepository movements(m_db->handle());
    InventoryEngine engine(m_db->handle(), products, movements);

    const auto suggestions = engine.reorderSuggestions();
    QCOMPARE(suggestions.size(), 1);
    QCOMPARE(suggestions.first().name, QString("Playera polo negra"));
}

QTEST_MAIN(InventoryEngineTest)
#include "InventoryEngineTest.moc"
