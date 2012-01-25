#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "ScanDetector.h"
#include "PortEntryValidator.h"

#include <QtDebug>
#include <QSettings>
#include <QTimer>
#include <QStringBuilder>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow), detector(0), listenOnStartup(false), minimzeToTray(false), enableNotifications(false), startMinimized(false)
{
    //Initialize GUI
    this->ui->setupUi(this);

    //Setup our tray icon
    this->createTrayIcon();

    //Prevent the user from entering crazy things in the "ports" field with a nice input validator
    this->ui->portEntryLine->setValidator(new PortEntryValidator(this));

    //QSettings uses these values to save/restore settings
    QCoreApplication::setOrganizationName("Spencer Clark");
    QCoreApplication::setApplicationName("Port Scan Detector");
    this->restoreSettings();

    //If our settings say to listen on startup, go ahead and start listening
    if (this->listenOnStartup)
        this->ui->startButton->setChecked(true);

    //If our settings say to start minimized, do so
    if (this->startMinimized)
        this->setWindowState(Qt::WindowMinimized);
}

MainWindow::~MainWindow()
{
    //Save settings with QSettings
    this->saveSettings();

    //Hide and destroy the tray icon
    this->trayIcon->hide();
    delete this->trayIcon;

    //Clean up GUI
    delete ui;
}

void MainWindow::changeEvent(QEvent *e)
{
    //Call the default implementation to do its thing
    QMainWindow::changeEvent(e);

    //Now, handle the custom stuff we care about
    switch (e->type())
    {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;

    case QEvent::WindowStateChange:
        //If the window has just been minimized and our settings say to minimize to tray, do so
        if (this->windowState() & Qt::WindowMinimized)
        {
            if (this->minimzeToTray)
                QTimer::singleShot(5,this,SLOT(hide()));
        }
        //When the user clicks the tray icon to restore the window, make it end up on top
        else if (!(this->windowState() & Qt::WindowMinimized))
        {
            this->activateWindow();
            this->raise();
        }
        break;

    default:
        break;
    }
}

//private slot
void MainWindow::on_listenOnStartCheckbox_toggled(bool checked)
{
    this->listenOnStartup = checked;
}

//private slot
void MainWindow::on_minimizeToTrayCheckbox_toggled(bool checked)
{
    this->minimzeToTray = checked;
}

//private slot
void MainWindow::on_enableNotificationsCheckbox_toggled(bool checked)
{
    this->enableNotifications = checked;
}

//private slot
void MainWindow::on_startMinimizedCheckbox_toggled(bool checked)
{
    this->startMinimized = checked;
}

//private slot
void MainWindow::on_startButton_toggled(bool checked)
{
    if (checked && this->detector == 0)
    {
        QList<quint16> ports;
        QStringList strings = this->ui->portEntryLine->text().split(',',QString::SkipEmptyParts);
        foreach(QString string, strings)
        {
            quint16 val = string.toUShort();
            ports.append(val);
        }

        this->detector = new ScanDetector(ports,2,30,this);
        connect(detector,
                SIGNAL(startedMonitoring()),
                this,
                SLOT(handleMonitoringStarted()));
        connect(detector,
                SIGNAL(stoppedMonitoring()),
                this,
                SLOT(handleMonitoringStopped()));
        connect(detector,
                SIGNAL(error(QString)),
                this,
                SLOT(handleMonitoringError(QString)));
        connect(detector,
                SIGNAL(scanDetected(QHostAddress,QList<quint16>)),
                this,
                SLOT(handleScanDetected(QHostAddress,QList<quint16>)));

        detector->startMonitoring();
    }
    else if (detector != 0)
    {
        detector->stopMonitoring();
    }
}

//private slot
void MainWindow::restoreSettings()
{
    QSettings settings;
    if (settings.contains("ports"))
        this->ui->portEntryLine->setText(settings.value("ports").toString());
    if (settings.contains("listenOnStartup"))
        this->ui->listenOnStartCheckbox->setChecked(settings.value("listenOnStartup").toBool());
    if (settings.contains("minimizeToTray"))
        this->ui->minimizeToTrayCheckbox->setChecked(settings.value("minimizeToTray").toBool());
    if (settings.contains("enableNotifications"))
        this->ui->enableNotificationsCheckbox->setChecked(settings.value("enableNotifications").toBool());
    if (settings.contains("startMinimized"))
        this->ui->startMinimizedCheckbox->setChecked(settings.value("startMinimized").toBool());
    if (settings.contains("geometry"))
        this->setGeometry(settings.value("geometry").toRect());
}

//private slot
void MainWindow::saveSettings()
{
    QSettings settings;
    settings.setValue("ports",this->ui->portEntryLine->text());
    settings.setValue("listenOnStartup",this->ui->listenOnStartCheckbox->isChecked());
    settings.setValue("minimizeToTray",this->ui->minimizeToTrayCheckbox->isChecked());
    settings.setValue("enableNotifications",this->ui->enableNotificationsCheckbox->isChecked());
    settings.setValue("startMinimized",this->ui->startMinimizedCheckbox->isChecked());
    settings.setValue("geometry",this->geometry());
}

//private slot
void MainWindow::logMessage(const QString &msg,bool notify)
{
    QString toShow = QDateTime::currentDateTime().toString() % " " % msg % "";
    this->ui->logTextEdit->appendPlainText(toShow);

    if (this->enableNotifications && notify)
        this->trayIcon->showMessage("Port Scan Detector",toShow,QSystemTrayIcon::Information,5000);
}

//private slot
void MainWindow::handleMonitoringStarted()
{
    this->logMessage("Started Monitoring",
                     false);
}

//private slot
void MainWindow::handleMonitoringStopped()
{
    this->logMessage("Stopped Monitoring",
                     false);
    this->detector->deleteLater();
    this->detector = 0;
}

//private slot
void MainWindow::handleMonitoringError(QString err)
{
    this->logMessage("Monitor returned error:" + err);
    this->detector->deleteLater();
    this->detector = 0;
}

//private slot
void MainWindow::handleScanDetected(QHostAddress host, QList<quint16> ports)
{
    QString msg = "Scan detected. Host:" + host.toString() + " Port(s):";

    foreach(quint16 port, ports)
        msg += QString::number(port) + " ";

    this->logMessage(msg);
}


//private
void MainWindow::createTrayIcon()
{
    this->trayIcon = new QSystemTrayIcon(QIcon(":/images/target-green.png"));
    this->trayIcon->setToolTip(QCoreApplication::applicationName());
    this->trayIcon->show();

    connect(this->trayIcon,
            SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this,
            SLOT(showNormal()));
}



