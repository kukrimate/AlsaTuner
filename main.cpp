#include "alsatunergui.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    AlsaTunerGUI w;
    w.show();

    return a.exec();
}
