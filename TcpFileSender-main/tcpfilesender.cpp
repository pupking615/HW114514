#include "tcpfilesender.h"
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QDataStream>
#include <QHostAddress>
#include <QDebug>
#include <QIODevice>

TcpFileSender::TcpFileSender(QWidget *parent)
    : QDialog(parent),
    loadSize(1024 * 4), totalBytes(0), bytesWritten(0), bytesToWrite(0),
    bytesReceived(0), totalReceivedBytes(0)
{
    // 初始化客戶端 UI
    clientProgressBar = new QProgressBar;
    clientStatusLabel = new QLabel(QStringLiteral("客戶端就緒"));

    ipLabel = new QLabel(QStringLiteral("  IP  :"));
    portLabel = new QLabel(QStringLiteral("Port:"));
    ipLineEdit = new QLineEdit("");
    portLineEdit = new QLineEdit("");
    portLineEdit->setValidator(new QIntValidator(1, 65535, this));

    startButton = new QPushButton(QStringLiteral("開始"));
    quitButton = new QPushButton(QStringLiteral("退出"));
    openButton = new QPushButton(QStringLiteral("開檔"));

    startButton->setEnabled(false);

    QVBoxLayout *ipPortLayout = new QVBoxLayout;
    QHBoxLayout *ipLayout = new QHBoxLayout;
    QHBoxLayout *portLayout = new QHBoxLayout;

    ipLayout->addWidget(ipLabel);
    ipLayout->addWidget(ipLineEdit);

    portLayout->addWidget(portLabel);
    portLayout->addWidget(portLineEdit);

    ipPortLayout->addLayout(ipLayout);
    ipPortLayout->addLayout(portLayout);

    QVBoxLayout *clientLayout = new QVBoxLayout;
    clientLayout->addWidget(clientProgressBar);
    clientLayout->addWidget(clientStatusLabel);
    clientLayout->addLayout(ipPortLayout);
    clientLayout->addStretch(1);
    clientLayout->addWidget(openButton);
    clientLayout->addWidget(startButton);
    clientLayout->addWidget(quitButton);

    // 初始化伺服器 UI
    serverProgressBar = new QProgressBar;
    serverStatusLabel = new QLabel(QStringLiteral("等待連接..."));
    startServerButton = new QPushButton(QStringLiteral("開始接收"));
    quitServerButton = new QPushButton(QStringLiteral("退出"));

    // 新增伺服器的 IP 和 Port 設定欄位
    serverIpLabel = new QLabel(QStringLiteral("伺服器 IP :"));
    serverPortLabel = new QLabel(QStringLiteral("伺服器 Port:"));
    serverIpLineEdit = new QLineEdit(""); // 用戶輸入伺服器 IP
    serverPortLineEdit = new QLineEdit(""); // 用戶輸入伺服器 Port
    serverPortLineEdit->setValidator(new QIntValidator(1, 65535, this));

    // 設置伺服器 UI 佈局
    QVBoxLayout *serverIpPortLayout = new QVBoxLayout;
    QHBoxLayout *serverIpLayout = new QHBoxLayout;
    QHBoxLayout *serverPortLayout = new QHBoxLayout;

    serverIpLayout->addWidget(serverIpLabel);
    serverIpLayout->addWidget(serverIpLineEdit);

    serverPortLayout->addWidget(serverPortLabel);
    serverPortLayout->addWidget(serverPortLineEdit);

    serverIpPortLayout->addLayout(serverIpLayout);
    serverIpPortLayout->addLayout(serverPortLayout);

    QVBoxLayout *serverLayout = new QVBoxLayout;
    serverLayout->addWidget(serverProgressBar);
    serverLayout->addWidget(serverStatusLabel);
    serverLayout->addLayout(serverIpPortLayout); // 新增 IP 和 Port 設定區
    serverLayout->addWidget(startServerButton);
    serverLayout->addWidget(quitServerButton);

    // 創建各個頁面並設定佈局
    QWidget *clientTabWidget = new QWidget;
    clientTabWidget->setLayout(clientLayout);

    QWidget *serverTabWidget = new QWidget;
    serverTabWidget->setLayout(serverLayout);

    // 創建 TabWidget 並新增標籤頁
    tabWidget = new QTabWidget;
    tabWidget->addTab(clientTabWidget, QStringLiteral("傳送檔案"));
    tabWidget->addTab(serverTabWidget, QStringLiteral("接收檔案"));

    // 主佈局
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(tabWidget);
    setLayout(mainLayout);

    setWindowTitle(QStringLiteral("檔案傳送程式"));

    // 訊號與槽的連接
    connect(openButton, &QPushButton::clicked, this, &TcpFileSender::openFile);
    connect(startButton, &QPushButton::clicked, this, &TcpFileSender::start);
    connect(&tcpClient, &QTcpSocket::connected, this, &TcpFileSender::startTransfer);
    connect(&tcpClient, &QTcpSocket::bytesWritten, this, &TcpFileSender::updateClientProgress);
    connect(quitButton, &QPushButton::clicked, this, &TcpFileSender::close);

    connect(startServerButton, &QPushButton::clicked, this, &TcpFileSender::startServer);
    connect(quitServerButton, &QPushButton::clicked, this, &TcpFileSender::close);
}

TcpFileSender::~TcpFileSender()
{
    if (localFile) {
        localFile->close();
        delete localFile;
    }
    if (receivedFile) {
        receivedFile->close();
        delete receivedFile;
    }
}

void TcpFileSender::openFile()
{
    fileName = QFileDialog::getOpenFileName(this);
    if (!fileName.isEmpty())
        startButton->setEnabled(true);
}

void TcpFileSender::start()
{
    startButton->setEnabled(false);
    bytesWritten = 0;
    clientStatusLabel->setText(QStringLiteral("連接中..."));

    QString ip = ipLineEdit->text();
    quint16 port = portLineEdit->text().toUShort();
    tcpClient.connectToHost(QHostAddress(ip), port);
}

void TcpFileSender::startTransfer()
{
    localFile = new QFile(fileName);
    if (!localFile->open(QFile::ReadOnly))
    {
        QMessageBox::warning(this, QStringLiteral("應用程式"),
                             QStringLiteral("無法讀取 %1:\n%2.")
                                 .arg(fileName)
                                 .arg(localFile->errorString()));
        return;
    }

    totalBytes = localFile->size();
    QDataStream sendOut(&outBlock, QIODevice::WriteOnly);
    sendOut.setVersion(QDataStream::Qt_4_6);
    QString currentFile = fileName.right(fileName.size() - fileName.lastIndexOf("/") - 1);
    sendOut << qint64(0) << qint64(0) << currentFile;
    totalBytes += outBlock.size();

    sendOut.device()->seek(0);
    sendOut << totalBytes << qint64((outBlock.size() - sizeof(qint64) * 2));
    bytesToWrite = totalBytes - tcpClient.write(outBlock);
    clientStatusLabel->setText(QStringLiteral("已連接"));
    outBlock.resize(0);
}

void TcpFileSender::updateClientProgress(qint64 numBytes)
{
    bytesWritten += (int)numBytes;
    if (bytesToWrite > 0)
    {
        outBlock = localFile->read(qMin(bytesToWrite, loadSize));
        bytesToWrite -= (int)tcpClient.write(outBlock);
        outBlock.resize(0);
    }
    else
    {
        localFile->close();
    }

    clientProgressBar->setMaximum(totalBytes);
    clientProgressBar->setValue(bytesWritten);
    clientStatusLabel->setText(QStringLiteral("已傳送 %1 Bytes").arg(bytesWritten));
}

void TcpFileSender::startServer()
{
    tcpServer = new QTcpServer(this);
    connect(tcpServer, &QTcpServer::newConnection, this, &TcpFileSender::acceptConnection);

    // 取得使用者輸入的 IP 和 Port
    QString serverIp = serverIpLineEdit->text();
    quint16 serverPort = serverPortLineEdit->text().toUShort();

    if (!tcpServer->listen(QHostAddress(serverIp), serverPort)) {
        serverStatusLabel->setText(QStringLiteral("無法啟動伺服器"));
        return;
    }

    serverStatusLabel->setText(QStringLiteral("伺服器啟動中"));
}

void TcpFileSender::acceptConnection()
{
    tcpServerSocket = tcpServer->nextPendingConnection();
    connect(tcpServerSocket, &QTcpSocket::readyRead, this, &TcpFileSender::receiveFile);
    serverStatusLabel->setText(QStringLiteral("接收檔案中..."));
}

void TcpFileSender::receiveFile()
{
    QDataStream in(tcpServerSocket);
    in.setVersion(QDataStream::Qt_4_6);

    if (bytesReceived <= sizeof(qint64) * 2) {
        if ((tcpServerSocket->bytesAvailable() >= sizeof(qint64) * 2) && (fileName.isEmpty())) {
            in >> totalBytes >> bytesReceived;
            fileName = QString::fromUtf8(in.device()->readAll());
            receivedFile = new QFile(fileName);
            if (!receivedFile->open(QFile::WriteOnly)) {
                QMessageBox::warning(this, QStringLiteral("應用程式"),
                                     QStringLiteral("無法寫入檔案 %1:\n%2.")
                                         .arg(fileName)
                                         .arg(receivedFile->errorString()));
                return;
            }
        }
    }

    if (bytesReceived < totalBytes) {
        bytesReceived += tcpServerSocket->bytesAvailable();
        receivedFile->write(tcpServerSocket->readAll());
        serverProgressBar->setMaximum(totalBytes);
        serverProgressBar->setValue(bytesReceived);
        serverStatusLabel->setText(QStringLiteral("已接收 %1 Bytes").arg(bytesReceived));
    } else {
        receivedFile->close();
        tcpServerSocket->close();
        serverStatusLabel->setText(QStringLiteral("檔案接收完畢"));
    }
}


