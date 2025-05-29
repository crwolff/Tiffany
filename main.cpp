#include "mainwindow.h"

#include <QApplication>
#include <QImageReader>

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("Tiffany");
    QCoreApplication::setOrganizationDomain("example.com");
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setApplicationName("Tiffany6");   // Format of font string changed
    QImageReader::setAllocationLimit(0);
#else
    QCoreApplication::setApplicationName("Tiffany");
#endif
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
