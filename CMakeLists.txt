cmake_minimum_required(VERSION 3.21)
project(Inventario VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

list(APPEND CMAKE_PREFIX_PATH "C:/Qt/6.10.3/msvc2022_64")

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql Network PrintSupport Svg SvgWidgets Test)
qt_standard_project_setup()

# CI pasa esto con la version real del tag (ej. -DINVENTARIO_VERSION_OVERRIDE=1.2.0)
# para que el .exe que se publica quede con la version correcta. En
# builds locales de desarrollo se usa la version fija de project() de
# arriba.
if(DEFINED INVENTARIO_VERSION_OVERRIDE)
    string(REPLACE "." ";" _inventario_version_parts "${INVENTARIO_VERSION_OVERRIDE}")
    list(GET _inventario_version_parts 0 PROJECT_VERSION_MAJOR)
    list(GET _inventario_version_parts 1 PROJECT_VERSION_MINOR)
    list(GET _inventario_version_parts 2 PROJECT_VERSION_PATCH)
    set(PROJECT_VERSION "${INVENTARIO_VERSION_OVERRIDE}")
endif()

configure_file(
    "${CMAKE_SOURCE_DIR}/src/version.h.in"
    "${CMAKE_BINARY_DIR}/generated/version.h"
    @ONLY
)

# InventarioCore: capas de datos (SQLite) y dominio (reglas de negocio),
# sin nada de UI. La comparten el ejecutable y las pruebas.
add_library(InventarioCore STATIC
    src/data/Models.h
    src/data/Models.cpp
    src/data/Database.h
    src/data/Database.cpp
    src/data/CategoryRepository.h
    src/data/CategoryRepository.cpp
    src/data/ProductRepository.h
    src/data/ProductRepository.cpp
    src/data/MovementRepository.h
    src/data/MovementRepository.cpp
    src/data/CutoffRepository.h
    src/data/CutoffRepository.cpp
    src/data/SupplierRepository.h
    src/data/SupplierRepository.cpp
    src/domain/InventoryEngine.h
    src/domain/InventoryEngine.cpp
    src/domain/CutoffEngine.h
    src/domain/CutoffEngine.cpp
    src/domain/ReportEngine.h
    src/domain/ReportEngine.cpp
    src/update/UpdateChecker.h
    src/update/UpdateChecker.cpp
    src/update/UpdateDownloader.h
    src/update/UpdateDownloader.cpp
)

target_include_directories(InventarioCore PUBLIC
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_BINARY_DIR}/generated"
)

target_link_libraries(InventarioCore PUBLIC
    Qt6::Sql
    Qt6::Network
)

qt_add_executable(Inventario WIN32
    src/main.cpp
    src/app/ThemeManager.h
    src/app/ThemeManager.cpp
    src/app/Animations.h
    src/app/Animations.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/ui/widgets/NavRail.h
    src/ui/widgets/NavRail.cpp
    src/ui/widgets/KpiCard.h
    src/ui/widgets/KpiCard.cpp
    src/ui/views/DashboardView.h
    src/ui/views/DashboardView.cpp
    src/ui/views/InventoryView.h
    src/ui/views/InventoryView.cpp
    src/ui/views/CutoffView.h
    src/ui/views/CutoffView.cpp
    src/ui/views/SupplierView.h
    src/ui/views/SupplierView.cpp
    src/ui/models/ProductTableModel.h
    src/ui/models/ProductTableModel.cpp
    src/ui/models/ProductFilterProxyModel.h
    src/ui/models/ProductFilterProxyModel.cpp
    src/ui/dialogs/ProductDialog.h
    src/ui/dialogs/ProductDialog.cpp
    src/ui/dialogs/MovementDialog.h
    src/ui/dialogs/MovementDialog.cpp
    src/ui/dialogs/MovementHistoryDialog.h
    src/ui/dialogs/MovementHistoryDialog.cpp
    src/ui/dialogs/SupplierDialog.h
    src/ui/dialogs/SupplierDialog.cpp
    src/ui/PdfExporter.h
    src/ui/PdfExporter.cpp
    src/ui/dialogs/UpdateDialog.h
    src/ui/dialogs/UpdateDialog.cpp
    resources/app.qrc
)

if(WIN32)
    target_sources(Inventario PRIVATE resources/app.rc)
endif()

target_include_directories(Inventario PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_BINARY_DIR}/generated"
)

target_link_libraries(Inventario PRIVATE
    InventarioCore
    Qt6::Widgets
    Qt6::Network
    Qt6::PrintSupport
    Qt6::Svg
    Qt6::SvgWidgets
)

set_target_properties(Inventario PROPERTIES
    OUTPUT_NAME "Inventario"
)

enable_testing()
add_subdirectory(tests)

option(INVENTARIO_BUILD_TOOLS "Compilar herramientas internas (regenerar assets/logo.ico, etc.)" OFF)
if(INVENTARIO_BUILD_TOOLS)
    qt_add_executable(svg2png tools/svg2png.cpp)
    target_link_libraries(svg2png PRIVATE Qt6::Gui Qt6::Svg)
endif()
