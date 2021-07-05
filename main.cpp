#include "application.h"

#include <QApplication>
#include <QStyleFactory>
#include <QFile>
#include "vls_common.h"

int main(int argc, char *argv[])
{
    QApplication::setOrganizationName("Alphacephei");
    QApplication::setApplicationName("vls");
    QApplication::setApplicationVersion(QString("%1.%2").arg(VLS::VLS_VERSION_MAJOR).arg(VLS::VLS_VERSION_MINOR));
    QApplication a(argc, argv);

    a.setStyle(QStyleFactory::create("Fusion"));

    Application w;
    return a.exec();
}
