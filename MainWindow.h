#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QHostAddress>

class ScanDetector;

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void changeEvent(QEvent *e);
    void closeEvent(QCloseEvent *);

private slots:
    void on_listenOnStartCheckbox_toggled(bool checked);

    void on_minimizeToTrayCheckbox_toggled(bool checked);

    void on_enableNotificationsCheckbox_toggled(bool checked);

    void on_startMinimizedCheckbox_toggled(bool checked);

    void on_startButton_toggled(bool checked);

    void on_exportLogButton_clicked();

    void on_clearLogButton_clicked();

    void restoreSettings();

    void saveSettings();

    void logMessage(const QString& msg, bool notify = true);

    void handleMonitoringStarted();

    void handleMonitoringStopped();

    void handleMonitoringError(QString err);

    void handleScanDetected(QHostAddress,QList<quint16>);

private:
    static QString logFilePath();
    void createTrayIcon();
    bool writeLogToFile(const QString& filePath, QString * errorMessage = 0);
    Ui::MainWindow *ui;

    ScanDetector * detector;

    bool listenOnStartup;
    bool minimzeToTray;
    bool enableNotifications;
    bool startMinimized;

    QSystemTrayIcon * trayIcon;
};

#endif // MAINWINDOW_H
