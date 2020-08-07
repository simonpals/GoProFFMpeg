#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QMap>

#include "CarouselSerial.h"

namespace Ui {
class MainWindow;
}

class FFmpegQT;
class GoProApiQT;
class LogsWindow;
class VideoEffects;
class QProcess;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
    void closeEvent(QCloseEvent *event) override;
    void printLogs(const QString &logs);

private slots:
    void on_serialPortOpenButton_clicked();
    void on_serialPortRefreshButton_clicked();
    void on_startButton_clicked();
    void on_saveToDeviceButton_clicked();
    void on_serialPortCloseButton_clicked();
    void on_stopButton_clicked();

    void onOpenSerialPort();
    void onCloseSerialPort();
    void onMotorStatusChange(CarouselSerial::MotorStatus status);

    void on_loadFromDeviceButton_clicked();
    void on_refreshMotorStatusButton_clicked();

    void on_connectCamera_clicked();
    void on_startCamera_clicked();
    void on_stopCamera_clicked();

    void on_convertionLog_clicked();
    void on_cbNoRecordingPerform_stateChanged(int);
    void on_startConvert_clicked();

    void on_playResultVideo_clicked();

public slots:
    void on_stopConvert_clicked();

private:
    Ui::MainWindow *ui;
    VideoEffects *ve;
    LogsWindow *lw;
    FFmpegQT *m_ffmpegQT;
    GoProApiQT *m_gpApi;
    CarouselSerial carouselSerial;
    QString m_strRecordVideoPath;
    bool m_bRecordStarted;
    volatile bool m_bConvertStarted;
    QTimer m_tRecordTimer;

    struct ffProcessInfo{
        int nCommand=-1;
        bool isFinished=false;
        QList<QVariant> commandArgs;
        QProcess *process=nullptr;
        QString commandName;
    };

    QMap<int, ffProcessInfo> m_ffTasksList;
    bool allProcessDone();
    void prepareProcessList();
    void clearProcessList();
    void performTasks();
    void removeTempFiles();

    void readSettings();
    void writeSettings();

    int timeStrToMS(const QString &timeStr);
    QString msToStrTime(int ms);
    void stopRecordTimer();
    void updateGuiForConvert(bool on);
    void updateCameraGui();
    void enableSpoiler();

    void fillPortsInfo();
    void enableGui();
    void updateGoProPanel(bool visible);
};

#endif // MAINWINDOW_H
