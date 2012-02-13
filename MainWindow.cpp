#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "ScanDetector.h"
#include "PortEntryValidator.h"

#include <QtDebug>
#include <QSettings>
#include <QTimer>
#include <QStringBuilder>
#include <QFileDialog>
#include <QDateTime>
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <QCloseEvent>

const quint32 saveSettingsIntervalMS = 5 * 60 * 1000; //Five minutes

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

    //Write settings (and more importantly, log data) to disk every once in a while
    QTimer * timer = new QTimer(this);
    connect(timer,
            SIGNAL(timeout()),
            this,
            SLOT(saveSettings()));
    timer->start(saveSettingsIntervalMS);
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
            //The timer is a hack
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

void MainWindow::closeEvent(QCloseEvent * event)
{
    //If we aren't detecting, just close normally
    if (this->detector == 0)
    {
        QMainWindow::closeEvent(event);
        return;
    }

    //If we're detecting, don't close
    event->ignore();

    //Toggle the "start" button to "off" so that our detector will stop.
    this->on_startButton_toggled(false);

    //Try again -- detector should stop by then
    QTimer::singleShot(10,this,SLOT(close()));
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

void MainWindow::on_exportLogButton_clicked()
{
    QString filePath = QFileDialog::getSaveFileName(this,
                                                    "Select output file for log export",
                                                    QString(),
                                                    "*.txt");
    //If the filePath is empty, the user probably cancelled the dialog
    if (filePath.isEmpty())
        return;

    //Try to write --- show an error otherwise
    QString errorMessage;
    if (!this->writeLogToFile(filePath,&errorMessage))
    {
        qWarning() << errorMessage;
        QMessageBox::warning(this,
                             "Failed to export log",
                             errorMessage);
        return;
    }

    //Show a message saying the export succeeded
    const QString message = "Log exported successfully.";
    qDebug() << message;
    QMessageBox::information(this,
                             "Log Export",
                             message);
}

void MainWindow::on_clearLogButton_clicked()
{
    this->ui->logTextEdit->clear();
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

    //Load the log from disk if it exists
    QFile fp(this->logFilePath());
    if (fp.exists())
    {
        //Try to open the file
        if (!fp.open(QFile::ReadOnly))
        {
            const QString error = "Failed to open log file for reading.";
            qWarning() << error;
            QMessageBox::warning(this,
                                 "Warning",
                                 error);
        }

        const QByteArray bytes = fp.readAll();
        const QString string (bytes);

        //Make sure we read ALL the bytes
        if (bytes.size() < fp.size())
        {
            const QString error = "Failed to read complete log file";
            qWarning() << error;
            QMessageBox::warning(this,
                                 "Warning",
                                 error);
        }
        else
            this->ui->logTextEdit->setPlainText(string);
    }
}

//private slot
void MainWindow::saveSettings()
{
    //Save the GUI settings
    QSettings settings;
    settings.setValue("ports",this->ui->portEntryLine->text());
    settings.setValue("listenOnStartup",this->ui->listenOnStartCheckbox->isChecked());
    settings.setValue("minimizeToTray",this->ui->minimizeToTrayCheckbox->isChecked());
    settings.setValue("enableNotifications",this->ui->enableNotificationsCheckbox->isChecked());
    settings.setValue("startMinimized",this->ui->startMinimizedCheckbox->isChecked());
    settings.setValue("geometry",this->geometry());

    //Save the current log to disk
    QString errorMessage;
    if (!this->writeLogToFile(this->logFilePath(),&errorMessage))
    {
        qWarning() << errorMessage;
        QMessageBox::warning(this,
                             "Warning",
                             errorMessage);
    }
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

//private static
QString MainWindow::logFilePath()
{
    return QDir::homePath() % QDir::separator() % QCoreApplication::applicationName() % "-log.txt";
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

bool MainWindow::writeLogToFile(const QString &filePath, QString *errorMessage)
{
    //Try to open the file for writing. Return an error on failure
    QFile fp(filePath);
    if (!fp.open(QFile::ReadWrite | QFile::Truncate))
    {
        if (errorMessage)
            *errorMessage = "Failed to open " % filePath % " to write the log. Error:" % fp.errorString();
        return false;
    }

    //Try to write the data. Return an error on failure.
    QByteArray writeBytes;
    writeBytes += this->ui->logTextEdit->toPlainText();

    if (fp.write(writeBytes) < writeBytes.size())
    {
        if (errorMessage)
            *errorMessage = "Failed to completely write log file. Error:" % fp.errorString();
        return false;
    }

    //Shouldn't be strictly necessary
    fp.close();

    return true;
}
