#include "application.h"

#include <QApplication>
#include <QStyleFactory>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication::setOrganizationName("VLS");
    QApplication::setApplicationName("vls");
    QApplication::setApplicationVersion("1.0");
    QApplication a(argc, argv);

    a.setStyle(QStyleFactory::create("Fusion"));

    Application w;
    w.show();
    return a.exec();
}
