#include "ffmpegstreamer.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    FFmpegStreamer w;
    w.show();
    return a.exec();
}
