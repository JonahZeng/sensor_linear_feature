#include "inc\mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setFont(QFont("Microsoft YaHei UI", 10));
    MainWindow w;
    w.show();

    return a.exec();
}
