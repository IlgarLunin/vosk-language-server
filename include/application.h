#pragma once

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class Application; }
QT_END_NAMESPACE

class Application : public QMainWindow
{
    Q_OBJECT

public:
    Application(QWidget *parent = nullptr);
    ~Application();

private slots:
    void on_pbRunCode_clicked();

private:
    Ui::Application *ui;
};
