#include "application.h"

#include <QApplication>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFile styleFile(":/style.css");
    styleFile.open(QFile::ReadOnly);
    QString style = styleFile.readAll();
    styleFile.close();
    a.setStyleSheet(style);

    Application w;
    w.show();
    return a.exec();
}
