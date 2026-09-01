#include "ui/MainWindow.h"

#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QSvgWidget>
#include <QVBoxLayout>

#include "version.h"

namespace ui {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setObjectName("AppRoot");
    setWindowTitle(APP_NAME);
    resize(960, 640);

    setCentralWidget(buildCentralWidget());
}

QWidget* MainWindow::buildCentralWidget() {
    auto* root = new QWidget(this);
    root->setObjectName("AppRoot");

    auto* rootLayout = new QVBoxLayout(root);
    rootLayout->setAlignment(Qt::AlignCenter);

    auto* card = new QWidget(root);
    card->setObjectName("SurfaceCard");
    card->setFixedWidth(360);

    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(40, 40, 40, 36);
    cardLayout->setSpacing(14);
    cardLayout->setAlignment(Qt::AlignHCenter);

    auto* logo = new QSvgWidget(":/logo.svg", card);
    logo->setFixedSize(88, 88);
    cardLayout->addWidget(logo, 0, Qt::AlignHCenter);

    auto* title = new QLabel(APP_NAME, card);
    title->setObjectName("TitleLabel");
    title->setAlignment(Qt::AlignHCenter);
    cardLayout->addWidget(title);

    auto* subtitle = new QLabel("Inventario de uniformes y papeleria", card);
    subtitle->setObjectName("SubtitleLabel");
    subtitle->setAlignment(Qt::AlignHCenter);
    cardLayout->addWidget(subtitle);

    cardLayout->addSpacing(10);

    auto* startButton = new QPushButton("Comenzar", card);
    startButton->setObjectName("PrimaryButton");
    startButton->setCursor(Qt::PointingHandCursor);
    cardLayout->addWidget(startButton, 0, Qt::AlignHCenter);

    cardLayout->addSpacing(6);

    auto* version = new QLabel(QString("v%1").arg(APP_VERSION_STRING), card);
    version->setObjectName("VersionLabel");
    version->setAlignment(Qt::AlignHCenter);
    cardLayout->addWidget(version);

    rootLayout->addWidget(card);

    return root;
}

void MainWindow::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);

    if (m_introPlayed) {
        return;
    }
    m_introPlayed = true;

    auto* effect = new QGraphicsOpacityEffect(centralWidget());
    centralWidget()->setGraphicsEffect(effect);

    auto* fadeIn = new QPropertyAnimation(effect, "opacity", this);
    fadeIn->setDuration(220);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->setEasingCurve(QEasingCurve::OutCubic);
    fadeIn->start(QAbstractAnimation::DeleteWhenStopped);
}

} // namespace ui
