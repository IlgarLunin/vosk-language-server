#include <QDebug>

#include "application.h"
#include "./ui_application.h"


Application::Application(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Application)
{
    ui->setupUi(this);
}

Application::~Application()
{
    delete ui;
}


void Application::on_pbRunCode_clicked()
{

}

