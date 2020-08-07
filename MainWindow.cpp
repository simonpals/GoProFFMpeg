#include <QSerialPortInfo>
#include <QMessageBox>
#include <QDebug>
#include <QFileDialog>
#include <QTime>
#include <QDir>
#include <QThread>
#include <QTextEdit>

#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "goproapi/goproapi.h"
#include "ffmpegqt.h"
#include "spoiler.h"
#include "logswindow.h"
#include "videoeffects.h"
#include "ui_videoeffects.h"
#include "settings.h"

void logCallBack(QString log, void *arg) {
    MainWindow *mw = reinterpret_cast<MainWindow*>(arg);
    mw->printLogs(log);
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    ve(nullptr),
    lw(new LogsWindow(nullptr)),
    m_bRecordStarted(false),
    m_bConvertStarted(false)
{
    ui->setupUi(this);

    ui->widget_1->setVisible(false);
    ui->widget_2->setVisible(false);

    setWindowFlags( (windowFlags() | Qt::CustomizeWindowHint) & ~Qt::WindowMaximizeButtonHint);
    setFixedWidth(width());

    lw->setMainWindow(this);
    ui->serialPortCloseButton->setEnabled(false);

    fillPortsInfo();
    enableSpoiler();
    readSettings();

    stopRecordTimer();
    m_tRecordTimer.setSingleShot(false);
    m_tRecordTimer.setInterval(400);
    m_tRecordTimer.setTimerType(Qt::TimerType::PreciseTimer);
    m_tRecordTimer.setProperty("begin_time", QVariant::fromValue(QTime::currentTime().msecsSinceStartOfDay()));

    m_ffmpegQT = new FFmpegQT;
    m_ffmpegQT->setLogsCallback(&logCallBack, reinterpret_cast<void*>(this));
    m_ffmpegQT->setCancellationVar(&m_bConvertStarted);
    m_gpApi = new GoProApiQT;

    adjustSize();

    _putenv("SDL_AUDIODRIVER=DirectSound");

    connect(&m_tRecordTimer, &QTimer::timeout, [=]()
    {
        ui->recordIndicator->setVisible(!ui->recordIndicator->isVisible());
        ui->timeRecordIndicator->setVisible(true);
        int recTime = QTime::currentTime().msecsSinceStartOfDay() - m_tRecordTimer.property("begin_time").toInt();
        ui->timeRecordIndicator->setText(msToStrTime(recTime));
    });

    connect(&carouselSerial, &CarouselSerial::serialPortOpenned, this, &MainWindow::onOpenSerialPort);
    connect(&carouselSerial, &CarouselSerial::serialPortClosed, this, &MainWindow::onCloseSerialPort);
    connect(&carouselSerial, &CarouselSerial::motosStatusChanged, this, &MainWindow::onMotorStatusChange);
    connect(m_gpApi, &GoProApiQT::connectionChanged, this, &MainWindow::updateGoProPanel);
}

MainWindow::~MainWindow()
{
    stopRecordTimer();
    writeSettings();
    delete m_ffmpegQT;
    delete m_gpApi;
    delete lw;
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    m_bConvertStarted = false;
    lw->close();
    QMainWindow::closeEvent(event);
}

void MainWindow::printLogs(const QString &logs)
{
    lw->getLogsList()->append(logs);
}

void MainWindow::fillPortsInfo()
{
    ui->serialPortInfoListBox->clear();

    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos)
    {
        ui->serialPortInfoListBox->addItem(info.portName());
    }
}

void MainWindow::enableGui()
{
    if (!carouselSerial.testCommand())
    {
        QMessageBox::warning(this, tr("Ошибка"), tr("Устройство не найдено"));
        carouselSerial.closeSerialPort();
        return;
    }

    on_loadFromDeviceButton_clicked();

    onMotorStatusChange(carouselSerial.getStatus());

    ui->serialPortOpenButton->setEnabled(true);
    ui->serialPortCloseButton->setEnabled(true);
    ui->serialPortOpenButton->setVisible(false);

    ui->refreshMotorStatusButton->setEnabled(true);
    ui->loadFromDeviceButton->setEnabled(true);
    ui->saveToDeviceButton->setEnabled(true);
    ui->startButton->setEnabled(true);
    ui->stopButton->setEnabled(false);
    ui->timeSpinBox->setEnabled(true);
    ui->speedSpinBox->setEnabled(true);
}

void MainWindow::updateGoProPanel(bool visible)
{
//    if(visible)
//        ui->macLabel->setText(m_gpApi->getSessionMAC());
//    else
//        ui->macLabel->setText("");

    stopRecordTimer();
    ui->startCamera->setEnabled(visible);
    ui->stopCamera->setEnabled(visible);
    if(visible)
        ui->connectionCammeraStatus->setText(tr("Подключено"));
    else
        ui->connectionCammeraStatus->setText(tr("Отключено"));

//    ui->loadFromCamera->setEnabled(visible);
}

void MainWindow::on_serialPortOpenButton_clicked()
{
    carouselSerial.openSerialPort(ui->serialPortInfoListBox->currentText());
}

void MainWindow::on_serialPortRefreshButton_clicked()
{
    fillPortsInfo();
}

void MainWindow::on_startButton_clicked()
{
    carouselSerial.start();
}

void MainWindow::on_saveToDeviceButton_clicked()
{
    bool result = carouselSerial.setSpeed(ui->speedSpinBox->value());
    result &= carouselSerial.setTime(ui->timeSpinBox->value());

    if (result)
    {
        QMessageBox::information(this, tr("Информация"), tr("Настройки успешно сохранены"));
    }
    else
    {
        QMessageBox::warning(this, tr("Ошибка"), tr("Не удалось сохранить настройки"));
    }
}

void MainWindow::on_serialPortCloseButton_clicked()
{
    carouselSerial.closeSerialPort();
}

void MainWindow::on_stopButton_clicked()
{
    carouselSerial.stop();
}

void MainWindow::onOpenSerialPort()
{
    ui->serialPortOpenButton->setEnabled(false);

    ui->label_4->setEnabled(false);
    ui->serialPortRefreshButton->setEnabled(false);
    ui->serialPortInfoListBox->setEnabled(false);

    QTimer::singleShot(6000, this, [=](){
        enableGui();
    });
}

void MainWindow::onCloseSerialPort()
{
    ui->serialPortOpenButton->setEnabled(true);
    ui->serialPortCloseButton->setEnabled(false);
    ui->serialPortOpenButton->setVisible(true);

    ui->refreshMotorStatusButton->setEnabled(false);
    ui->loadFromDeviceButton->setEnabled(false);
    ui->saveToDeviceButton->setEnabled(false);
    ui->startButton->setEnabled(false);
    ui->stopButton->setEnabled(false);
    ui->timeSpinBox->setEnabled(false);
    ui->speedSpinBox->setEnabled(false);

    ui->label_4->setEnabled(true);
    ui->serialPortRefreshButton->setEnabled(true);
    ui->serialPortInfoListBox->setEnabled(true);
}

void MainWindow::onMotorStatusChange(CarouselSerial::MotorStatus status)
{
    qDebug() << status;

    if (status == CarouselSerial::STOPPED_MOTOR_STATUS)
    {
        ui->startButton->setEnabled(true);
        ui->stopButton->setEnabled(false);
    }
    else
    {
        ui->startButton->setEnabled(false);
        ui->stopButton->setEnabled(true);
    }

    if (status == CarouselSerial::STOPPED_MOTOR_STATUS)
    {
        ui->motorStatusLabel->setText(tr("Остановлено"));
    }
    else if (status == CarouselSerial::INIT_START_MOTOR_STATUS)
    {
        ui->motorStatusLabel->setText(tr("Начало инициализации"));
    }
    else if (status == CarouselSerial::INIT_STOP_MOTOR_STATUS)
    {
        ui->motorStatusLabel->setText(tr("Конец инициализации"));
    }
    else if (status == CarouselSerial::SMOOTH_START_MOTOR_STATUS)
    {
        ui->motorStatusLabel->setText(tr("Плавный пуск"));
    }
    else if (status == CarouselSerial::SMOOTH_STOP_MOTOR_STATUS)
    {
        ui->motorStatusLabel->setText(tr("Плавная остановка"));
    }
    else if (status == CarouselSerial::ROTATED_MOTOR_STATUS)
    {
        ui->motorStatusLabel->setText(tr("Вращение"));
    }
    else
    {
        ui->motorStatusLabel->setText(tr("Неизвестный"));
    }
}

void MainWindow::on_loadFromDeviceButton_clicked()
{
    int rpm = carouselSerial.getSpeed();
    int time = carouselSerial.getTime();

    if (rpm == -1 || time == -1)
    {
        QMessageBox::warning(this, tr("Ошибка"), tr("Не удалось загрузить настройки"));
        return;
    }

    ui->speedSpinBox->setValue(rpm);
    ui->timeSpinBox->setValue(time);
}

void MainWindow::on_refreshMotorStatusButton_clicked()
{
    onMotorStatusChange(carouselSerial.getStatus());
}

void MainWindow::on_connectCamera_clicked()
{
    ui->connectionCammeraStatus->setText(tr("Подключение..."));
    m_gpApi->Connect();
}

void MainWindow::on_startCamera_clicked()
{
    if(ve->ui->originalVideoPath->text().isEmpty())
        QMessageBox::information(nullptr, tr("Путь отсутствует"), tr("Вам нужно указать Папку c исходным видео"));
    else if(!m_tRecordTimer.isActive())
    {
        ui->cbNoRecordingPerform->setChecked(false);
        m_bRecordStarted = true;

        m_gpApi->StartVideo();

        if(m_gpApi->CameraIsBusy())
        {
            if(ui->recordTime->isChecked()
                    && ui->recordTimeSpin->value()>0.0)
            {
                QTimer::singleShot(ui->recordTimeSpin->value()*1000.0, this, [=](){
                    on_stopCamera_clicked();
                });
            }
            m_tRecordTimer.setProperty("begin_time", QVariant::fromValue(QTime::currentTime().msecsSinceStartOfDay()));
            m_tRecordTimer.start();
        }
    }
}

void MainWindow::on_stopCamera_clicked()
{
    bool wasPalyed = m_gpApi->CameraIsBusy() && m_bRecordStarted;
    m_gpApi->StopVideo();
    stopRecordTimer();
    m_bRecordStarted = false;
    if(wasPalyed)
    {
        m_gpApi->LoadFromCamera(m_strRecordVideoPath, ve->ui->originalVideoPath->text(),
                                ve->ui->removeFromCamera->isChecked());
        if(m_strRecordVideoPath.isEmpty())
            QMessageBox::information(nullptr, tr("Ошибка загрузки"), tr("Видео с GoPro не было загружено"));
        on_startConvert_clicked();
    }
}

void MainWindow::enableSpoiler()
{
    Spoiler *spoiler = new Spoiler("Видеоэффекты", 300, ui->horizontalSpoilerLayout->widget());
    ve = new VideoEffects(spoiler);
    auto * anyLayout = new QVBoxLayout();
    anyLayout->setMargin(1);
    anyLayout->addWidget(ve);
    spoiler->setContentLayout(*anyLayout);
    ui->horizontalSpoilerLayout->addWidget(spoiler);

    QObject::connect(spoiler, &Spoiler::animationFinished, [=]() {
        adjustSize();
    });
}

void MainWindow::on_convertionLog_clicked()
{
    lw->open();
}

void MainWindow::on_cbNoRecordingPerform_stateChanged(int s)
{
    QString dir;
    bool checked = Qt::Checked == s;
    if(checked)
        dir = ui->anotherVideoPath->text().isEmpty() ?
                    QFileDialog::getExistingDirectory(nullptr, ("Выбрать выходную папку"), QDir::currentPath()) :
                    ui->anotherVideoPath->text();
//    ui->anotherVideoPath->setEnabled(checked);
    ui->anotherVideoPath->setText(dir);
    updateCameraGui();
}

void MainWindow::updateCameraGui()
{
    bool visible = (ui->cbNoRecordingPerform->isChecked() && !ui->anotherVideoPath->text().isEmpty())
            || !m_strRecordVideoPath.isEmpty();

//    ui->cbSlowDown->setEnabled(visible);
//    ui->slowDownBegin->setEnabled(visible);
//    ui->slowDownEnd->setEnabled(visible);
//    ui->slowDownValue->setEnabled(visible);
//    ui->playResultVideo->setEnabled(visible);

//    ui->recordTime->setEnabled(visible);
//    ui->recordTimeSpin->setEnabled(visible);
    ui->startConvert->setEnabled(visible);
//    ui->playConverted->setEnabled(visible);

//    ve->ui->audioOverlay->setEnabled(visible);
//    ve->ui->audioOverlayDialog->setEnabled(visible);
//    ve->ui->audioOverlayPath->setEnabled(visible);
//    ve->ui->cropVideo1x1->setEnabled(visible);
//    ve->ui->imageOverlay->setEnabled(visible);
//    ve->ui->imageOverlayDialog->setEnabled(visible);
//    ve->ui->imageOverlayPath->setEnabled(visible);
//    ve->ui->reverseVideo->setEnabled(visible);
//    ve->ui->videoOverlay->setEnabled(visible);
//    ve->ui->videoOverlayDialog->setEnabled(visible);
//    ve->ui->videoOverlayPath->setEnabled(visible);
//    ve->ui->comboCompression->setEnabled(visible);
//    ve->ui->introVideo->setEnabled(visible);
//    ve->ui->introVideoDialog->setEnabled(visible);
//    ve->ui->introVideoPath->setEnabled(visible);
//    ve->ui->outroVideo->setEnabled(visible);
//    ve->ui->outroVideoDialog->setEnabled(visible);
//    ve->ui->outroVideoPath->setEnabled(visible);
//    ve->ui->removeFromCamera->setEnabled(visible);
}

void MainWindow::on_startConvert_clicked()
{
    if(m_tRecordTimer.isActive() || m_bRecordStarted)
    {
        QMessageBox::information(nullptr, tr("Приложение занято"), tr("Сначала необходимо остановить запись видео"));
        return;
    }

    if(ve->ui->savingVideoPath->text().isEmpty())
    {
        QMessageBox::information(nullptr, tr("Путь отсутствует"), tr("Необходимо указать Папку для сохранения видео"));
        return;
    }

    if(ve->ui->audioOverlay->isChecked() && ve->ui->audioOverlayPath->text().isEmpty())
    {
        QMessageBox::information(nullptr, tr("Путь отсутствует"), tr("Вам нужно указать путь наложения аудио"));
        return;
    }

    if(ve->ui->videoOverlay->isChecked() && ve->ui->videoOverlayPath->text().isEmpty())
    {
        QMessageBox::information(nullptr, tr("Путь отсутствует"), tr("Вам нужно указать путь для наложения видео"));
        return;
    }

    if(ve->ui->imageOverlay->isChecked() && ve->ui->imageOverlayPath->text().isEmpty())
    {
        QMessageBox::information(nullptr, tr("Путь отсутствует"), tr("Вам нужно указать путь для наложения рисунка"));
        return;
    }

    if(ve->ui->introVideo->isChecked() && ve->ui->introVideoPath->text().isEmpty())
    {
        QMessageBox::information(nullptr, tr("Путь отсутствует"), tr("Вам нужно указать путь для добавления интро"));
        return;
    }

    if(ve->ui->outroVideo->isChecked() && ve->ui->outroVideoPath->text().isEmpty())
    {
        QMessageBox::information(nullptr, tr("Путь отсутствует"), tr("Вам нужно указать путь для добавления аутро"));
        return;
    }

    QFileInfoList files;
    if(ui->cbNoRecordingPerform->isChecked())
    {
        QDir directory(ui->anotherVideoPath->text());
        if(!directory.exists())
        {
            QMessageBox::information(nullptr, tr("Папка отсутствует"), tr("Вам нужно указать путь к видео для обработки"));
            return;
        }
        files = directory.entryInfoList(QStringList(),QDir::Files);
        if(files.isEmpty())
        {
            QMessageBox::information(nullptr, tr("Папка пуста"), tr("В выбранной папке нет видео для обработки"));
            return;
        }
    }
    else
    {
        if(!QFile::exists(m_strRecordVideoPath))
        {
            QMessageBox::information(nullptr, tr("Исходный файл отсутствует"), tr("Необходимо указать путь к исходному видеофайлу для дальнейшей обработки"));
            return;
        }
        files << QFileInfo(m_strRecordVideoPath);
    }


    m_bConvertStarted = true;
    on_convertionLog_clicked();
    updateGuiForConvert(false);

    foreach(QFileInfo sourcePath, files)
    {
        if(!sourcePath.exists())
        {
            QMessageBox::information(nullptr, tr("Исходный файл отсутствует"), tr("Не найден исходный видеофайл для дальнейшей обработки"));
            break;
        }

//        QFileInfo fileInfo(sourcePath);
        QString srcFileName = sourcePath.fileName();
        QString destPath = ve->ui->savingVideoPath->text()+"/"+srcFileName;
        if(sourcePath.absoluteFilePath()==destPath)
        {
            destPath.insert(destPath.lastIndexOf("."), "_out");
        }
        if(QFile::exists(destPath))
        {
            QFile::remove(destPath);
        }

        m_ffmpegQT->setInputFile(sourcePath.absoluteFilePath());
        m_ffmpegQT->setOutputFile(destPath);
        m_ffmpegQT->setConvertionQuality(ve->ui->convertQuality->currentIndex());
        lw->setPerformedPath(sourcePath.absoluteFilePath());

        prepareProcessList();
        while(m_bConvertStarted && !allProcessDone())
        {
            performTasks();
        }
        clearProcessList();

        if(!m_bConvertStarted)
            break;
    }
    lw->setPerformedPath("");
    lw->setPerformedCommand("");
    updateGuiForConvert(true);
    m_bConvertStarted = false;
}

static QString tempFile1 = "temp1.mp4";
static QString tempFile2 = "temp2.mp4";

void MainWindow::performTasks()
{
    QString origInPath = m_ffmpegQT->getInputFile();
    QString origOutPath = m_ffmpegQT->getOutputFile();
    int nCount = m_ffTasksList.count(), nTask=0;

    removeTempFiles();

    m_ffmpegQT->checkAndInsertSilence(origInPath);

    for(ffProcessInfo task : m_ffTasksList.values())
    {
        if(!task.isFinished)
        {
            if(nTask==0 && nCount > 1)
                m_ffmpegQT->setOutputFile(tempFile1);
            else if(nTask == nCount-1 && nCount > 1)
            {
                m_ffmpegQT->setInputFile(tempFile2);
                m_ffmpegQT->setOutputFile(origOutPath);
            }
            else if(nCount > 1)
            {
                m_ffmpegQT->setInputFile(tempFile2);
                m_ffmpegQT->setOutputFile(tempFile1);
            }

            lw->setPerformedCommand(task.commandName);

            switch (task.nCommand) {
            case FFmpegQT::Crop1x1 : {
                m_ffmpegQT->crop1x1();
                break;
            }
            case FFmpegQT::Reverse : {
                m_ffmpegQT->reverse();
                break;
            }
            case FFmpegQT::Compress : {
                Q_ASSERT(task.commandArgs.count()==1);
                m_ffmpegQT->changeResolution(task.commandArgs.at(0));
                break;
            }
            case FFmpegQT::AddIntro : {
                Q_ASSERT(task.commandArgs.count()==1);
                m_ffmpegQT->addIntro(task.commandArgs.at(0));
                break;
            }
            case FFmpegQT::AddOutro : {
                Q_ASSERT(task.commandArgs.count()==1);
                m_ffmpegQT->addOutro(task.commandArgs.at(0));
                break;
            }
            case FFmpegQT::AudioOverlay : {
                Q_ASSERT(task.commandArgs.count()==1);
                m_ffmpegQT->addAudioOverlay(task.commandArgs.at(0));
                break;
            }
            case FFmpegQT::VideoOverlay : {
                Q_ASSERT(task.commandArgs.count()==1);
                m_ffmpegQT->addVideoOverlay(task.commandArgs.at(0));
                break;
            }
            case FFmpegQT::ImageOverlay : {
                Q_ASSERT(task.commandArgs.count()==1);
                m_ffmpegQT->addImageOverlay(task.commandArgs.at(0));
                break;
            }
            case FFmpegQT::SlowDown : {
                Q_ASSERT(task.commandArgs.count()==1);
                m_ffmpegQT->slowDown(task.commandArgs.at(0));
                break;
            }
            case FFmpegQT::SlowDownPart : {
                Q_ASSERT(task.commandArgs.count()==3);
                m_ffmpegQT->slowDownPart(task.commandArgs.at(0), task.commandArgs.at(1), task.commandArgs.at(2));
                break;
            }
            }

            task.isFinished = true;
            m_ffTasksList[task.nCommand] = task;

            QString tf1 = tempFile1;
            tempFile1 = tempFile2;
            tempFile2 = tf1;

            nTask++;

            if(!m_bConvertStarted)
                break;
        }
    }

    removeTempFiles();
}

void MainWindow::removeTempFiles()
{
    if(QFile::exists(tempFile1))
        QFile::remove(tempFile1);
    if(QFile::exists(tempFile2))
        QFile::remove(tempFile2);
}

void MainWindow::readSettings()
{
    if(!Settings::instance().isValid)
        return;

    int ind = SETTINGS.serialPortInfoLBIndex;
    if(ui->serialPortInfoListBox->count() > ind && ind > -1)
        ui->serialPortInfoListBox->setCurrentIndex(ind);

    ui->timeSpinBox->setValue(SETTINGS.timeSpinValue);
    ui->speedSpinBox->setValue(SETTINGS.speedSpinValue);
    ui->cbSlowDown->setChecked(SETTINGS.cbSlowDownValue);
    ui->slowDownValue->setValue(SETTINGS.slowDownValue);
    ui->slowDownBegin->setValue(SETTINGS.slowDownBegin);
    ui->slowDownEnd->setValue(SETTINGS.slowDownEnd);
    ui->anotherVideoPath->setText(SETTINGS.anotherVideoPath);
    ui->cbNoRecordingPerform->setChecked(SETTINGS.cbNoRecordingPerform);
    ui->recordTime->setChecked(SETTINGS.cbRecordTime);
    ui->recordTimeSpin->setValue(SETTINGS.recordTimeValue);
    ve->ui->originalVideoPath->setText(SETTINGS.originalVideoPath);
    ve->ui->savingVideoPath->setText(SETTINGS.savingVideoPath);
    ve->ui->audioOverlay->setChecked(SETTINGS.audioOverlay);
    ve->ui->audioOverlayPath->setText(SETTINGS.audioOverlayPath);
    ve->ui->imageOverlay->setChecked(SETTINGS.imageOverlay);
    ve->ui->imageOverlayPath->setText(SETTINGS.imageOverlayPath);
    ve->ui->videoOverlay->setChecked(SETTINGS.videoOverlay);
    ve->ui->videoOverlayPath->setText(SETTINGS.videoOverlayPath);
    ve->ui->introVideo->setChecked(SETTINGS.introVideo);
    ve->ui->introVideoPath->setText(SETTINGS.introVideoPath);
    ve->ui->outroVideo->setChecked(SETTINGS.outroVideo);
    ve->ui->outroVideoPath->setText(SETTINGS.outroVideoPath);
    ve->ui->removeFromCamera->setChecked(SETTINGS.removeFromCamera);
    ve->ui->cropVideo1x1->setChecked(SETTINGS.cropVideo1x1);
    ve->ui->reverseVideo->setChecked(SETTINGS.reverseVideo);

    ind = SETTINGS.comboCompressionIndex;
    if(ve->ui->comboCompression->count() > ind && ind > -1)
        ve->ui->comboCompression->setCurrentIndex(ind);
}

void MainWindow::writeSettings()
{
    SETTINGS.serialPortInfoLBIndex = ui->serialPortInfoListBox->currentIndex();
    SETTINGS.timeSpinValue = ui->timeSpinBox->value();
    SETTINGS.speedSpinValue = ui->speedSpinBox->value();
    SETTINGS.cbSlowDownValue = ui->cbSlowDown->isChecked();
    SETTINGS.slowDownValue = ui->slowDownValue->value();
    SETTINGS.slowDownBegin = ui->slowDownBegin->value();
    SETTINGS.slowDownEnd = ui->slowDownEnd->value();
    SETTINGS.cbNoRecordingPerform = ui->cbNoRecordingPerform->isChecked();
    SETTINGS.anotherVideoPath = ui->anotherVideoPath->text();
    SETTINGS.cbRecordTime = ui->recordTime->isChecked();
    SETTINGS.recordTimeValue = ui->recordTimeSpin->value();
    SETTINGS.originalVideoPath = ve->ui->originalVideoPath->text();
    SETTINGS.savingVideoPath = ve->ui->savingVideoPath->text();
    SETTINGS.audioOverlay = ve->ui->audioOverlay->isChecked();
    SETTINGS.audioOverlayPath = ve->ui->audioOverlayPath->text();
    SETTINGS.imageOverlay = ve->ui->imageOverlay->isChecked();
    SETTINGS.imageOverlayPath = ve->ui->imageOverlayPath->text();
    SETTINGS.videoOverlay = ve->ui->videoOverlay->isChecked();
    SETTINGS.videoOverlayPath = ve->ui->videoOverlayPath->text();
    SETTINGS.introVideo = ve->ui->introVideo->isChecked();
    SETTINGS.introVideoPath = ve->ui->introVideoPath->text();
    SETTINGS.outroVideo = ve->ui->outroVideo->isChecked();
    SETTINGS.outroVideoPath = ve->ui->outroVideoPath->text();
    SETTINGS.removeFromCamera = ve->ui->removeFromCamera->isChecked();
    SETTINGS.cropVideo1x1 = ve->ui->cropVideo1x1->isChecked();
    SETTINGS.reverseVideo = ve->ui->reverseVideo->isChecked();
    SETTINGS.comboCompressionIndex = ve->ui->comboCompression->currentIndex();
}

int MainWindow::timeStrToMS(const QString &timeStr)
{
    QStringList timeLst = timeStr.split(":");
    return timeLst.at(2).toInt() * 1000 + timeLst.at(1).toInt() * 60000 + timeLst.at(0).toInt() * 3600000;
}

QString MainWindow::msToStrTime(int ms)
{
    int hours = ms/3600000;
    int minutes = (ms%3600000)/60000;
    int seconds = ((ms%3600000)%60000)/1000;
    return QString("%1:%2:%3").arg(hours).arg(minutes).arg(seconds);
}

void MainWindow::stopRecordTimer()
{
    m_tRecordTimer.stop();
    ui->recordIndicator->setVisible(false);
    ui->timeRecordIndicator->setVisible(false);
    ui->timeRecordIndicator->setText("00:00:00");
}

void MainWindow::updateGuiForConvert(bool on)
{
    this->setEnabled(on);    
    if(on)
        lw->hide();
    else
        lw->getLogsList()->clear();
}

void MainWindow::on_stopConvert_clicked()
{
    m_bConvertStarted = false;
}

bool MainWindow::allProcessDone()
{
    for(ffProcessInfo ffCommand : m_ffTasksList.values())
    {
        if(!ffCommand.isFinished)
            return false;
    }

    return true;
}

static QMap<int, double> slowDownSliderValues {{1,0.25},{2,0.5},{3,1},{4,2},{5,4},{6,6},{7,8}};

void MainWindow::prepareProcessList()
{
    if(ve->ui->audioOverlay->isChecked())
        m_ffTasksList.insert(FFmpegQT::AudioOverlay, {FFmpegQT::AudioOverlay, false, {QVariant(ve->ui->audioOverlayPath->text())}, nullptr, "Добавление музыки..."});
    if(ve->ui->videoOverlay->isChecked())
        m_ffTasksList.insert(FFmpegQT::VideoOverlay, ffProcessInfo
        {FFmpegQT::VideoOverlay, false, {QVariant(ve->ui->videoOverlayPath->text())}, nullptr, "Добавление анимации..."});
    if(ve->ui->imageOverlay->isChecked())
        m_ffTasksList.insert(FFmpegQT::ImageOverlay, ffProcessInfo
        {FFmpegQT::ImageOverlay, false, {QVariant(ve->ui->imageOverlayPath->text())}, nullptr, "Добавление рисунка..."});
    if(ve->ui->introVideo->isChecked())
        m_ffTasksList.insert(FFmpegQT::AddIntro, {FFmpegQT::AddIntro, false, {QVariant(ve->ui->introVideoPath->text())}, nullptr, "Добавление интро..."});
    if(ve->ui->outroVideo->isChecked())
        m_ffTasksList.insert(FFmpegQT::AddOutro, {FFmpegQT::AddOutro, false, {QVariant(ve->ui->outroVideoPath->text())}, nullptr, "Добавление аутро..."});
    if(ve->ui->cropVideo1x1->isChecked())
        m_ffTasksList.insert(FFmpegQT::Crop1x1, {FFmpegQT::Crop1x1,false, {}, nullptr, "Обрезка кадра 1:1..."});
    if(ve->ui->reverseVideo->isChecked())
        m_ffTasksList.insert(FFmpegQT::Reverse, {FFmpegQT::Reverse,false, {}, nullptr, "Бумеранг (реверс)..."});
    if(ve->ui->comboCompression->currentIndex()>0)
        m_ffTasksList.insert(FFmpegQT::Compress, {FFmpegQT::Compress,false, {QVariant(ve->ui->comboCompression->currentIndex())}, nullptr, "Сжатие..."});
    if(ui->cbSlowDown->isChecked())
    {
        Q_ASSERT(slowDownSliderValues.keys().contains(ui->slowDownValue->value()));
        double slowDownValue = slowDownSliderValues[ui->slowDownValue->value()];
        int begin = static_cast<int>(ui->slowDownBegin->value() * 1000.0);
        int end = static_cast<int>(ui->slowDownEnd->value() * 1000.0);

        if(begin>0.0 || end>0.0)
        {
            if(begin < end)
                m_ffTasksList.insert(FFmpegQT::SlowDownPart, {FFmpegQT::SlowDownPart, false, {QVariant(slowDownValue),QVariant(begin),QVariant(end-begin)}, nullptr, "Замедление части..."});
        }
        else
        {
            m_ffTasksList.insert(FFmpegQT::SlowDown, {FFmpegQT::SlowDown, false, {QVariant(slowDownValue)}, nullptr, "Замедление всего видео..."});
        }
    }
}

void MainWindow::clearProcessList()
{
    for(ffProcessInfo ffCommand : m_ffTasksList.values())
    {
        if(ffCommand.process)
        {
            ffCommand.process->kill();
            delete ffCommand.process;
        }
        ffCommand.process = nullptr;
    }
    m_ffTasksList.clear();
}

void MainWindow::on_playResultVideo_clicked()
{
    m_ffmpegQT->openInSystemPlayer();
}
