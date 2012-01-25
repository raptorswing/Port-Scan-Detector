#include "ScanDetector.h"

#include <QTcpServer>
#include <QTcpSocket>

ScanDetector::ScanDetector(const QList<quint16>& listenPorts,
                           quint32 portHitThreshold,
                           quint32 timeoutSeconds,
                           QObject *parent) :
    QObject(parent),
    listenPorts(listenPorts),
    portHitThreshold(portHitThreshold),
    timeoutSeconds(timeoutSeconds),
    started(false)
{
}

bool ScanDetector::startMonitoring(QString *messages)
{
    if (this->started)
    {
        if (messages)
            *messages = "Already started";
        return false;
    }

    foreach(quint16 portNum, this->listenPorts)
    {
        QTcpServer * server = new QTcpServer(this);
        connect(this,
                SIGNAL(stoppedMonitoring()),
                server,
                SLOT(deleteLater()));
        connect(this,
                SIGNAL(error(QString)),
                server,
                SLOT(deleteLater()));
        connect(server,
                SIGNAL(newConnection()),
                this,
                SLOT(handleIncomingConnection()));

        if (!server->listen(QHostAddress::Any,portNum))
        {
            if (messages)
                *messages = server->errorString();
            this->error("Failed to listen on port" + QString::number(portNum));
            return false;
        }
    }

    this->startedMonitoring();
    this->started = true;
    return true;
}

bool ScanDetector::stopMonitoring(QString *messages)
{
    if (!this->started)
    {
        if (messages)
            *messages = "Not started";
        return false;
    }

    this->stoppedMonitoring();
    this->started = false;
    return true;
}

bool ScanDetector::isStarted() const
{
    return this->started;
}

//private slot
void ScanDetector::handleIncomingConnection()
{
    QObject * obj = QObject::sender();
    QTcpServer * server = qobject_cast<QTcpServer *>(obj);
    if (!server)
        return;

    while (server->hasPendingConnections())
    {
        QTcpSocket * sock = server->nextPendingConnection();

        QList<quint16> ports;
        ports.append(server->serverPort());
        this->scanDetected(sock->peerAddress(),ports);

        sock->write("<!>PSD<!>");
        sock->deleteLater();
    }

}
