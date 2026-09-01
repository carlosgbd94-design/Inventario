#pragma once

#include <QVector>
#include <QWidget>

#include "data/Models.h"

class QListWidget;
class QListWidgetItem;

namespace ui {

// Rail de navegacion fijo a la izquierda: Dashboard + una entrada por
// categoria (cargadas dinamicamente desde la base de datos) + boton
// para crear una categoria nueva.
class NavRail : public QWidget {
    Q_OBJECT

public:
    explicit NavRail(QWidget* parent = nullptr);

    void setCategories(const QVector<data::Category>& categories);
    void selectDashboard();

signals:
    void dashboardSelected();
    void categorySelected(const data::Category& category);
    void cutoffsSelected();
    void addCategoryRequested();

private:
    void rebuildList();
    void onItemChanged(QListWidgetItem* current, QListWidgetItem* previous);

    QListWidget* m_list = nullptr;
    QVector<data::Category> m_categories;
};

} // namespace ui
