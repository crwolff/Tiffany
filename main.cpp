#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("Tiffany");
    QCoreApplication::setOrganizationDomain("example.com");
    QCoreApplication::setApplicationName("Tiffany");
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
