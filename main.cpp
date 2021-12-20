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

    QPalette darkPalette = QPalette();
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(35, 35, 35));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, QColor(35, 35, 35));
    darkPalette.setColor(QPalette::Active, QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, Qt::darkGray);
    darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, Qt::darkGray);
    darkPalette.setColor(QPalette::Disabled, QPalette::Text, Qt::darkGray);
    darkPalette.setColor(QPalette::Disabled, QPalette::Light, QColor(53, 53, 53));

    a.setPalette(darkPalette);

    Application w;
    return a.exec();
}
