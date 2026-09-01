qt_add_executable(InventoryEngineTest InventoryEngineTest.cpp)
target_link_libraries(InventoryEngineTest PRIVATE InventarioCore Qt6::Test Qt6::Sql)
add_test(NAME InventoryEngineTest COMMAND InventoryEngineTest)

qt_add_executable(CutoffEngineTest CutoffEngineTest.cpp)
target_link_libraries(CutoffEngineTest PRIVATE InventarioCore Qt6::Test Qt6::Sql)
add_test(NAME CutoffEngineTest COMMAND CutoffEngineTest)

qt_add_executable(UpdateCheckerTest UpdateCheckerTest.cpp)
target_link_libraries(UpdateCheckerTest PRIVATE InventarioCore Qt6::Test Qt6::Network)
add_test(NAME UpdateCheckerTest COMMAND UpdateCheckerTest)
