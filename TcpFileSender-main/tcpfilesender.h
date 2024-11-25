#ifndef TCPFILESENDER_H
#define TCPFILESENDER_H

#include <QDialog>
#include <QTcpSocket>
#include <QTcpServer>
#include <QFile>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QIntValidator>
#include <QTabWidget> // 需要包含 QTabWidget 標頭檔

class TcpFileSender : public QDialog
{
    Q_OBJECT

public:
    explicit TcpFileSender(QWidget *parent = nullptr);
    ~TcpFileSender();

private slots:
    void openFile();
    void start();
    void startTransfer();
    void updateClientProgress(qint64 numBytes);
    void startServer();
    void acceptConnection();
    void receiveFile();

private:
    QTcpSocket tcpClient;
    QTcpServer *tcpServer;
    QTcpSocket *tcpServerSocket;
    QFile *localFile;
    QFile *receivedFile;

    QProgressBar *clientProgressBar;
    QLabel *clientStatusLabel;
    QLabel *ipLabel;
    QLabel *portLabel;
    QLineEdit *ipLineEdit;
    QLineEdit *portLineEdit;
    QPushButton *startButton;
    QPushButton *quitButton;
    QPushButton *openButton;

    QProgressBar *serverProgressBar;
    QLabel *serverStatusLabel;
    QLabel *serverIpLabel;
    QLabel *serverPortLabel;
    QLineEdit *serverIpLineEdit;
    QLineEdit *serverPortLineEdit;
    QPushButton *startServerButton;
    QPushButton *quitServerButton;

    QTabWidget *tabWidget; // 新增 tabWidget 變數

    QString fileName;
    QByteArray outBlock;

    qint64 totalBytes;
    qint64 bytesWritten;
    qint64 bytesToWrite;
    qint64 bytesReceived;
    qint64 totalReceivedBytes;

    const int loadSize;

};

#endif // TCPFILESENDER_H
