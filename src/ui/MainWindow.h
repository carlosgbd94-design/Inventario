#pragma once

#include <QMainWindow>
#include <QSqlDatabase>

#include "data/CategoryRepository.h"
#include "data/CutoffRepository.h"
#include "data/MovementRepository.h"
#include "data/ProductRepository.h"
#include "data/SupplierRepository.h"
#include "domain/CutoffEngine.h"
#include "domain/InventoryEngine.h"
#include "domain/ReportEngine.h"
#include "update/UpdateChecker.h"

class QStackedWidget;

namespace ui {

class NavRail;
class DashboardView;
class InventoryView;
class CutoffView;
class SupplierView;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QSqlDatabase& db, QWidget* parent = nullptr);

protected:
    void showEvent(QShowEvent* event) override;

private:
    QWidget* buildIntroPage();
    QWidget* buildShellPage();
    void buildMenuBar();
    void refreshNavCategories();
    void refreshDashboard();
    void onAddCategory();
    void checkForUpdates(bool manual);
    void onAbout();

    data::CategoryRepository m_categoryRepo;
    data::ProductRepository m_productRepo;
    data::MovementRepository m_movementRepo;
    data::CutoffRepository m_cutoffRepo;
    data::SupplierRepository m_supplierRepo;
    domain::InventoryEngine m_inventoryEngine;
    domain::CutoffEngine m_cutoffEngine;
    domain::ReportEngine m_reportEngine;

    QStackedWidget* m_pageStack = nullptr;
    QStackedWidget* m_contentStack = nullptr;
    NavRail* m_navRail = nullptr;
    DashboardView* m_dashboard = nullptr;
    InventoryView* m_inventoryView = nullptr;
    CutoffView* m_cutoffView = nullptr;
    SupplierView* m_supplierView = nullptr;

    bool m_introPlayed = false;
    bool m_manualUpdateCheckInFlight = false;
};

} // namespace ui
