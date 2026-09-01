#pragma once

#include <QMainWindow>

namespace ui {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void showEvent(QShowEvent* event) override;

private:
    QWidget* buildCentralWidget();

    bool m_introPlayed = false;
};

} // namespace ui
