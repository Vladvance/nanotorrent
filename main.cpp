#include "nt_gui.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    NT_GUI w;
    w.runCore();
    w.show();
    return a.exec();
}
