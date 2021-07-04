#include "application.h"

#include <QApplication>
#include <QStyleFactory>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication::setOrganizationName("Alphacephei");
    QApplication::setApplicationName("vls");
    QApplication::setApplicationVersion("2021.1");
    QApplication a(argc, argv);

    a.setStyle(QStyleFactory::create("Fusion"));

    Application w;
    w.show();
    return a.exec();
}
