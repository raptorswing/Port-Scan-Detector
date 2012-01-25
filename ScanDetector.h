#ifndef SCANDETECTOR_H
#define SCANDETECTOR_H

#include <QObject>
#include <QList>
#include <QtGlobal>
#include <QHostAddress>

class ScanDetector : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief
     *
     * @param listenPorts list of ports to listen on
     * @param portHitThreshold not used right now
     * @param timeoutSeconds not used right now
     * @param parent
     */
    explicit ScanDetector(const QList<quint16>& listenPorts,
                          quint32 portHitThreshold=2,
                          quint32 timeoutSeconds=30,
                          QObject *parent = 0);

    bool startMonitoring(QString * messages=0);

    bool stopMonitoring(QString * messages=0);

    bool isStarted() const;


signals:
    void scanDetected(const QHostAddress& scanner, const QList<quint16>& ports);

    void startedMonitoring();

    void stoppedMonitoring();

    void error(QString error);

    public slots:

private slots:
    void handleIncomingConnection();

private:
    QList<quint16> listenPorts;
    quint32 portHitThreshold;
    quint32 timeoutSeconds;
    bool started;
};

#endif // SCANDETECTOR_H
