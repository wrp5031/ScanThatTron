#include "mainwindow.h"
#include <QApplication>
#include <QtGui>
#include <QWidget>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    return a.exec();
}
