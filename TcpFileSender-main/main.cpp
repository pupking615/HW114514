#include <QApplication>
#include "tcpfilesender.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    TcpFileSender w;
    w.show();  // 顯示對話框
    return a.exec();
}
