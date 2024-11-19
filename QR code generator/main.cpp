#include "kuvatesti.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    kuvatesti w;
    w.show();
    return a.exec();
}
