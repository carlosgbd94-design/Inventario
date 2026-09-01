#include "ui/widgets/NavRail.h"

#include <QListWidget>
#include <QListWidgetItem>
#include <QVBoxLayout>

namespace ui {

namespace {
constexpr int kRoleKind = Qt::UserRole;
constexpr int kRoleCategoryId = Qt::UserRole + 1;

constexpr int kKindDashboard = 0;
constexpr int kKindCategory = 1;
constexpr int kKindAddCategory = 2;
} // namespace

NavRail::NavRail(QWidget* parent) : QWidget(parent) {
    setObjectName("NavRail");
    // Un QWidget personalizado no pinta background-color/border del QSS
    // por si solo; hay que decirle explicitamente que respete el estilo.
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedWidth(220);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 20, 12, 20);
    layout->setSpacing(4);

    m_list = new QListWidget(this);
    m_list->setObjectName("NavList");
    m_list->setFrameShape(QFrame::NoFrame);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setFocusPolicy(Qt::NoFocus);
    m_list->setCursor(Qt::PointingHandCursor);
    m_list->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // La lista debe llenar todo el rail (no solo el alto de sus items):
    // si no, se queda arrinconada arriba con un hueco vacio debajo y una
    // barra de scroll de sobra que nadie necesita en un menu tan corto.
    m_list->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    layout->addWidget(m_list, 1);

    connect(m_list, &QListWidget::currentItemChanged, this, &NavRail::onItemChanged);
    connect(m_list, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        if (item && item->data(kRoleKind).toInt() == kKindAddCategory) {
            emit addCategoryRequested();
        }
    });

    rebuildList();
}

void NavRail::setCategories(const QVector<data::Category>& categories) {
    m_categories = categories;
    rebuildList();
}

void NavRail::selectDashboard() {
    if (m_list->count() > 0) {
        m_list->setCurrentRow(0);
    }
}

void NavRail::rebuildList() {
    m_list->blockSignals(true);
    m_list->clear();

    auto* dashboardItem = new QListWidgetItem("Panel general", m_list);
    dashboardItem->setData(kRoleKind, kKindDashboard);

    for (const data::Category& category : m_categories) {
        auto* item = new QListWidgetItem(category.name, m_list);
        item->setData(kRoleKind, kKindCategory);
        item->setData(kRoleCategoryId, category.id);
    }

    auto* addItem = new QListWidgetItem("+  Nueva categoria", m_list);
    addItem->setData(kRoleKind, kKindAddCategory);
    addItem->setFlags(addItem->flags() & ~Qt::ItemIsSelectable);
    addItem->setForeground(QColor("#6B7080"));

    m_list->blockSignals(false);
    m_list->setCurrentRow(0);
}

void NavRail::onItemChanged(QListWidgetItem* current, QListWidgetItem* /*previous*/) {
    if (!current) {
        return;
    }

    const int kind = current->data(kRoleKind).toInt();
    if (kind == kKindDashboard) {
        emit dashboardSelected();
    } else if (kind == kKindCategory) {
        const qint64 categoryId = current->data(kRoleCategoryId).toLongLong();
        for (const data::Category& category : m_categories) {
            if (category.id == categoryId) {
                emit categorySelected(category);
                break;
            }
        }
    }
}

} // namespace ui
