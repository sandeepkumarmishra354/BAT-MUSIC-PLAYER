#include "music_player.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Music_Player w;
    w.show();
    return a.exec();
}
