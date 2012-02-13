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

    //Create a QTcpServer on each of our ports
    //Connect slots for notifying of connection/error and for cleanup
    foreach(quint16 portNum, this->listenPorts)
    {
        QTcpServer * server = new QTcpServer(this);

        //If the we emit the stopped monitoring signal, the QTcpServer should be deleted
        connect(this,
                SIGNAL(stoppedMonitoring()),
                server,
                SLOT(deleteLater()));

        //If we have an error, the QTcpServer should be deleted
        connect(this,
                SIGNAL(error(QString)),
                server,
                SLOT(deleteLater()));

        //If there's an incoming connection on a QTcpServer, we want to know about it
        connect(server,
                SIGNAL(newConnection()),
                this,
                SLOT(handleIncomingConnection()));

        //Try to listen on the server
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
        sock->flush();
        sock->deleteLater();
    }

}
