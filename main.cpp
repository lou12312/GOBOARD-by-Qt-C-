#include "goboard.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    GoBoard w;
    w.setWindowTitle("围棋");
    w.resize(800, 800);
    w.show();

    return a.exec();
}
