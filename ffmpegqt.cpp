#include "ffmpegqt.h"
#include <QDebug>
#include <QMessageBox>
#include <QFile>
#include <QMap>
#include <QTextStream>
#include <QApplication>
#include <QThread>

static bool kbNeedPresets = true;
static bool kbNeedChangeFPS = true;
static bool kbNeedChangeCRF = true;
static int knFPSValue = 30;
static int knCRFValue = 23;
static QString ksPresetName = "ultrafast";


static const QString tempAudio = "./temp_audio.MP4";
static const QString tempVideo = "./temp_video.MP4";
static const QString tempVideo2 = "./temp_video2.MP4";
static const QString tempVideo3 = "./temp_video3.MP4";
static const QString tempVideo4 = "./temp_video4.MP4";
static const QString tempVideo5 = "./temp_video5.MP4";
static QMap<double, QVector<double>> atempoMultiplier{{8.0, { 0.5, 0.5, 0.5 } }, {6.0, { 0.5, 0.5, 0.666666666 } }, {4.0, { 0.5, 0.5, 1.0 } }, {2.0, { 1.0, 0.5, 1.0 } }, {0.5, { 1.0, 2.0, 1.0 } }, {0.25, { 2.0, 2.0, 1.0 } }};

FFmpegQT::FFmpegQT(QObject *parent) : QObject(parent)
{
    m_logCB = nullptr;
    m_logCBargument = nullptr;
    m_playProcess = new QProcess;
    m_nCancellation = nullptr;
    m_nConvertionQuality = -1;
}

FFmpegQT::~FFmpegQT()
{
    m_playProcess->kill();
    delete m_playProcess;
}

void FFmpegQT::setInputFile(const QString &inFile)
{
    m_strInputFile = inFile;
}

void FFmpegQT::setOutputFile(const QString &outFile)
{
    m_strOutputFile = outFile;
}

void FFmpegQT::setConvertionQuality(int nValue)
{
    m_nConvertionQuality=nValue;

    if(nValue==0)//high quality
    {
        knFPSValue = 30;
        knCRFValue = 17;
        ksPresetName = "slower";
    }
    else if(nValue==1)//middle quality
    {
        knFPSValue = 30;
        knCRFValue = 23;
        ksPresetName = "fast";
    }
    else if(nValue==2)//low quality
    {
        knFPSValue = 20;
        knCRFValue = 30;
        ksPresetName = "ultrafast";
    }
}

void FFmpegQT::checkAndInsertSilence(const QString &path)
{
    if(!QFile::exists(path) || isMediaContainAudio(path)) return;
//    removeExtraTempMedia();
    QString silenceTemp = "temp_with_silence.mp4";
    if(QFile::exists(silenceTemp))
        QFile::remove(silenceTemp);
    QStringList sl;

    sl << "-f" << "lavfi";
    sl << "-i" << "anullsrc";
    sl << "-i" << path;
    sl << "-shortest" << "-c:v" << "copy";
    sl << "-c:a" << "aac";
    if(kbNeedChangeFPS) sl << "-r" << QString::number(knFPSValue);
    if(kbNeedChangeCRF) sl << "-crf" << QString::number(knCRFValue);
    if(kbNeedPresets) sl << "-preset" << ksPresetName;
    sl << silenceTemp;

    executeCommand(ffExecutable(), sl, nullptr, true);
    sl.clear();

    QFile::remove(path);
    QFile::copy(silenceTemp,path);
    if(QFile::exists(silenceTemp))
        QFile::remove(silenceTemp);
//    removeExtraTempMedia();
}

void FFmpegQT::slowDown(double slowVal)
{
    if(!checkResFiles(true) || slowVal==1.0) return;
    Q_ASSERT(atempoMultiplier.keys().contains(slowVal));

    QStringList sl;
    removeTempMedia();

    sl << "-i" << m_strInputFile;
    sl << "-filter:v" << "setpts="+QString::number(slowVal)+"*PTS";
//    sl << "-movflags" << "+faststart";
    if(kbNeedChangeFPS) sl << "-r" << QString::number(knFPSValue);
    if(kbNeedChangeCRF) sl << "-crf" << QString::number(knCRFValue);
    sl << "-an";
    if(kbNeedPresets) sl << "-preset" << ksPresetName;
    sl << tempVideo;

    executeCommand(ffExecutable(), sl, nullptr, true);
    sl.clear();

//    qDebug() << QString("atempo=%1,atempo=%2,atempo=%3").arg(atempoMultiplier[slowVal].at(0))
//            .arg(atempoMultiplier[slowVal].at(1)).arg(atempoMultiplier[slowVal].at(2));

    if(isMediaContainAudio(m_strInputFile))
    {
        sl << "-i" << m_strInputFile;
        sl << "-filter:a" << QString("atempo=%1,atempo=%2,atempo=%3").arg(atempoMultiplier[slowVal].at(0))
              .arg(atempoMultiplier[slowVal].at(1)).arg(atempoMultiplier[slowVal].at(2));
        sl << "-vn";
        if(kbNeedPresets) sl << "-preset" << ksPresetName;
        sl << tempAudio;

        executeCommand(ffExecutable(), sl, nullptr, true);
        sl.clear();

        sl << "-i" << tempVideo;
        sl << "-i" << tempAudio;
        sl << "-c" << "copy";
        if(kbNeedPresets) sl << "-preset" << ksPresetName;
        sl << "-shortest";
        sl << m_strOutputFile;

        executeCommand(ffExecutable(), sl, nullptr, true);
        sl.clear();
    }
    else
    {
        QFile::rename(tempVideo, m_strOutputFile);
    }
    removeTempMedia();
}

void FFmpegQT::slowDownPart(double slowVal, int msBegin, int msDuration)
{
    if(!checkResFiles(true) || slowVal==1.0) return;
    Q_ASSERT(atempoMultiplier.keys().contains(slowVal));

    int videoMode = 0;
    QStringList sl;
    removeExtraTempMedia();
    int thresholdMS = 100;

    bool hasAudio = isMediaContainAudio(m_strInputFile);

    if(msBegin > thresholdMS)
    {
        sl << "-i" << m_strInputFile;
        sl << "-ss" << QString::number(static_cast<double>(0.0));
        sl << "-t" << QString::number(static_cast<double>(msBegin/1000.0));
        if(kbNeedChangeFPS) sl << "-r" << QString::number(knFPSValue);
        if(kbNeedChangeCRF) sl << "-crf" << QString::number(knCRFValue);
        if(kbNeedPresets) sl << "-preset" << ksPresetName;
        sl << tempVideo2;
        executeCommand(ffExecutable(), sl, nullptr, true);
        sl.clear();
        videoMode=1;
    }

    sl << "-i" << m_strInputFile;
    sl << "-ss" << QString::number(static_cast<double>(msBegin/1000.0));
    sl << "-t" << QString::number(static_cast<double>(msDuration/1000.0));
    if(kbNeedChangeFPS) sl << "-r" << QString::number(knFPSValue);
    if(kbNeedChangeCRF) sl << "-crf" << QString::number(knCRFValue);
    if(kbNeedPresets) sl << "-preset" << ksPresetName;
    sl << tempVideo3;
    executeCommand(ffExecutable(), sl, nullptr, true);
    sl.clear();

    if(msBegin+msDuration<getVideoDurationMs()-thresholdMS)
    {
        sl << "-i" << m_strInputFile;
        sl << "-ss" << QString::number(static_cast<double>((msBegin+msDuration)/1000.0));
        if(kbNeedChangeFPS) sl << "-r" << QString::number(knFPSValue);
        if(kbNeedChangeCRF) sl << "-crf" << QString::number(knCRFValue);
        if(kbNeedPresets) sl << "-preset" << ksPresetName;
        sl << tempVideo4;
        executeCommand(ffExecutable(), sl, nullptr, true);
        sl.clear();
        videoMode+=2;
    }

    QString inputOld = m_strInputFile;
    QString outputOld = m_strOutputFile;
    setInputFile(tempVideo3);
    setOutputFile(tempVideo5);
    slowDown(slowVal);
    setInputFile(inputOld);
    setOutputFile(outputOld);

    if(hasAudio)
    {
        if(videoMode==0)
        {
            sl << "-i" << tempVideo5;
            sl << "-filter_complex" << "[0:v:0] [0:a:0] concat=n=1:v=1:a=1 [v] [a]";
        }
        else if(videoMode==1)
        {
            sl << "-i" << tempVideo2;
            sl << "-i" << tempVideo5;
            sl << "-filter_complex" << "[0:v:0] [0:a:0] [1:v:0] [1:a:0] concat=n=2:v=1:a=1 [v] [a]";
        }
        else if(videoMode==2)
        {
            sl << "-i" << tempVideo5;
            sl << "-i" << tempVideo4;
            sl << "-filter_complex" << "[0:v:0] [0:a:0] [1:v:0] [1:a:0] concat=n=2:v=1:a=1 [v] [a]";
        }
        else if(videoMode==3)
        {
            sl << "-i" << tempVideo2;
            sl << "-i" << tempVideo5;
            sl << "-i" << tempVideo4;
            sl << "-filter_complex" << "[0:v:0] [0:a:0] [1:v:0] [1:a:0] [2:v:0] [2:a:0] concat=n=3:v=1:a=1 [v] [a]";
        }
    }
    else
    {
        if(videoMode==0)
        {
            sl << "-i" << tempVideo5;
            sl << "-filter_complex" << "[0:v:0] concat=n=1:v=1 [v]";
        }
        else if(videoMode==1)
        {
            sl << "-i" << tempVideo2;
            sl << "-i" << tempVideo5;
            sl << "-filter_complex" << "[0:v:0] [1:v:0] concat=n=2:v=1 [v]";
        }
        else if(videoMode==2)
        {
            sl << "-i" << tempVideo5;
            sl << "-i" << tempVideo4;
            sl << "-filter_complex" << "[0:v:0] [1:v:0] concat=n=2:v=1 [v]";
        }
        else if(videoMode==3)
        {
            sl << "-i" << tempVideo2;
            sl << "-i" << tempVideo5;
            sl << "-i" << tempVideo4;
            sl << "-filter_complex" << "[0:v:0] [1:v:0] [2:v:0] concat=n=3:v=1 [v]";
        }
    }
    sl << "-map" << "[v]";
    if(hasAudio)
        sl << "-map" << "[a]";
    if(kbNeedPresets) sl << "-preset" << ksPresetName;
    sl << m_strOutputFile;

    executeCommand(ffExecutable(), sl, nullptr, true);
    sl.clear();

    removeExtraTempMedia();
}

void FFmpegQT::reverse()
{
    if(!checkResFiles(true)) return;
    QStringList sl;

    removeExtraTempMedia();

    sl << "-i" << m_strInputFile;
    if(kbNeedChangeFPS) sl << "-r" << QString::number(knFPSValue);
    if(kbNeedChangeCRF) sl << "-crf" << QString::number(knCRFValue);
    sl << "-vf" << "reverse" << "-af" << "areverse";
//    sl << "-movflags" << "+faststart";
    if(kbNeedPresets) sl << "-preset" << ksPresetName;
    sl << tempVideo2;

    executeCommand(ffExecutable(), sl, nullptr, true);
    sl.clear();

    addOutro(tempVideo2);

    removeExtraTempMedia();
}

void FFmpegQT::crop1x1()
{
    if(!checkResFiles(true)) return;
    QStringList sl;

    QSize s = getVideoRectSize();
    int size = std::min(s.width(),s.height());
    int x = s.width()-size;
    int y = s.height()-size;

    sl << "-i" << m_strInputFile;
    sl << "-filter:v" << "crop=in_w-"+QString::number(x)+":in_h-"+QString::number(y);
    sl << "-c:a" << "copy";
    if(kbNeedChangeFPS) sl << "-r" << QString::number(knFPSValue);
    if(kbNeedChangeCRF) sl << "-crf" << QString::number(knCRFValue);
//    sl << "-movflags" << "+faststart";
    if(kbNeedPresets) sl << "-preset" << ksPresetName;
    sl << m_strOutputFile;

    executeCommand(ffExecutable(), sl, nullptr, true);
    sl.clear();
}

void FFmpegQT::addIntro(const QString &introPath)
{
    if(!checkResFiles(true) || !QFile::exists(introPath)) return;
    QStringList sl;
    bool introHasAudio = isMediaContainAudio(introPath);

    removeTempMedia();

    QSize s = getVideoRectSize();
    sl << "-noautorotate";
    sl << "-i" << introPath;
    sl << "-filter_complex" << "[0:v]scale="+QString::number(s.width())+":"+QString::number(s.height())
          +":force_original_aspect_ratio=decrease,pad="+QString::number(s.width())
          +":"+QString::number(s.height())+":(ow-iw)/2:(oh-ih)/2[v0]; "+
          (introHasAudio?"[v0][0:a]concat=n=1:v=1:a=1[v][a]":"[v0]concat=n=1:v=1[v]");
    sl << "-map" << "[v]";
    if(introHasAudio) sl << "-map" << "[a]";
    if(kbNeedChangeFPS) sl << "-r" << QString::number(knFPSValue);
    if(kbNeedChangeCRF) sl << "-crf" << QString::number(knCRFValue);
//    sl << "-vf" << "scale="+QString::number(s.width())+":"+QString::number(s.height());
    if(kbNeedPresets) sl << "-preset" << ksPresetName;
    sl << tempVideo;

    executeCommand(ffExecutable(), sl, nullptr, true);
    sl.clear();

    checkAndInsertSilence(tempVideo);

    sl << "-i" << tempVideo;
    sl << "-noautorotate";
    sl << "-i" << m_strInputFile;
    if(kbNeedChangeFPS) sl << "-r" << QString::number(knFPSValue);
    if(kbNeedChangeCRF) sl << "-crf" << QString::number(knCRFValue);
    sl << "-filter_complex" << "[0:v:0] [0:a:0] [1:v:0] [1:a:0] concat=n=2:v=1:a=1 [v] [a]";
    sl << "-map" << "[v]";
    sl << "-map" << "[a]";
//    sl << "-movflags" << "+faststart";
    if(kbNeedPresets) sl << "-preset" << ksPresetName;
    sl << m_strOutputFile;

    executeCommand(ffExecutable(), sl, nullptr, true);
    sl.clear();

    removeTempMedia();
}

void FFmpegQT::addOutro(const QString &outroPath)
{
    if(!checkResFiles(true) || !QFile::exists(outroPath)) return;
    QStringList sl;
    QString outroNewPath=outroPath;
    bool outroHasAudio = isMediaContainAudio(outroPath);
    removeTempMedia();

//    QString inputOld = m_strInputFile;
//    setInputFile(outroPath);
//    QSize sOutro = getVideoRectSize();
//    setInputFile(inputOld);
    QSize sInput = getVideoRectSize();

//    if(sOutro!=sInput)
    {
        sl << "-noautorotate";
        sl << "-i" << outroPath;
        sl << "-filter_complex" << "[0:v]scale="+QString::number(sInput.width())+":"+QString::number(sInput.height())
              +":force_original_aspect_ratio=decrease,pad="+QString::number(sInput.width())
              +":"+QString::number(sInput.height())+":(ow-iw)/2:(oh-ih)/2[v0]; "+
              (outroHasAudio?"[v0][0:a]concat=n=1:v=1:a=1[v][a]":"[v0]concat=n=1:v=1[v]");
        sl << "-map" << "[v]";
        if(outroHasAudio) sl << "-map" << "[a]";
        if(kbNeedChangeFPS) sl << "-r" << QString::number(knFPSValue);
        if(kbNeedChangeCRF) sl << "-crf" << QString::number(knCRFValue);
//        sl << "-vf" << "scale="+QString::number(sInput.width())+":"+QString::number(sInput.height());
        if(kbNeedPresets) sl << "-preset" << ksPresetName;
        sl << tempVideo;        
        executeCommand(ffExecutable(), sl, nullptr, true);
        sl.clear();
        checkAndInsertSilence(tempVideo);
        outroNewPath = tempVideo;
    }

    sl << "-noautorotate";
    sl << "-i" << m_strInputFile;
    sl << "-i" << outroNewPath;
    if(kbNeedChangeFPS) sl << "-r" << QString::number(knFPSValue);
    if(kbNeedChangeCRF) sl << "-crf" << QString::number(knCRFValue);
    sl << "-filter_complex" << "[0:v:0] [0:a:0] [1:v:0] [1:a:0] concat=n=2:v=1:a=1 [v] [a]";
    sl << "-map" << "[v]";
    sl << "-map" << "[a]";
//    sl << "-movflags" << "+faststart";
    if(kbNeedPresets) sl << "-preset" << ksPresetName;
    sl << m_strOutputFile;

    executeCommand(ffExecutable(), sl, nullptr, true);
    sl.clear();

    removeTempMedia();
}

void FFmpegQT::changeResolution(int newResolution)
{
    if(!checkResFiles(true) && (newResolution > 0 && newResolution < 3)) return;
    QStringList sl;

    sl << "-i" << m_strInputFile;
    if(newResolution==1)
        sl << "-s" << "hd720" << "-c:v" << "libx264";
    else
        sl << "-s" << "hd1080" << "-c:v" << "libx264";
    sl << "-c:a" << "aac";
    sl << "-strict" << "-2";
    if(kbNeedChangeFPS) sl << "-r" << QString::number(knFPSValue);
    if(kbNeedChangeCRF) sl << "-crf" << QString::number(knCRFValue);
//    sl << "-movflags" << "+faststart";
    if(kbNeedPresets) sl << "-preset" << ksPresetName;
    sl << m_strOutputFile;

    executeCommand(ffExecutable(), sl, nullptr, true);
    sl.clear();
}

void FFmpegQT::addAudioOverlay(const QString &audioPath)
{
    if(!checkResFiles(true) || !QFile::exists(audioPath)) return;
    QStringList sl;

    sl << "-i" << m_strInputFile;
    sl << "-i" << audioPath;

//    if(isMediaContainAudio())
//    {
//        sl << "-filter_complex" << "amix=inputs=2:duration=first:dropout_transition=3";
//    }
//    else
    {
        sl << "-c:v" << "copy";
        sl << "-map" << "0:v:0";
        sl << "-map" << "1:a:0";
        sl << "-t" << QString::number(static_cast<double>(getVideoDurationMs()/1000.0));
    }

    if(kbNeedChangeFPS) sl << "-r" << QString::number(knFPSValue);
    if(kbNeedChangeCRF) sl << "-crf" << QString::number(knCRFValue);
    if(kbNeedPresets) sl << "-preset" << ksPresetName;
    sl << m_strOutputFile;

    executeCommand(ffExecutable(), sl, nullptr, true);
    sl.clear();
}

void FFmpegQT::addImageOverlay(const QString &imagePath)
{
    if(!checkResFiles(true) || !QFile::exists(imagePath)) return;
    QStringList sl;

    sl << "-i" << m_strInputFile;
    sl << "-i" << imagePath;
    sl << "-filter_complex" << "[0:v][1:v] overlay=(W-w)/2:(H-h)/2";
    sl << "-c:a" << "copy";
    if(kbNeedChangeFPS) sl << "-r" << QString::number(knFPSValue);
    if(kbNeedChangeCRF) sl << "-crf" << QString::number(knCRFValue);
    if(kbNeedPresets) sl << "-preset" << ksPresetName;
    sl << m_strOutputFile;

    executeCommand(ffExecutable(), sl, nullptr, true);
    sl.clear();
}

void FFmpegQT::addVideoOverlay(const QString &videoPath)
{
    if(!checkResFiles(true) || !QFile::exists(videoPath)) return;
    QStringList sl;
    removeTempMedia();
    QString videoOverlaySrc = m_strInputFile;

    if(isMediaContainAudio(videoPath))
    {
        bool ok=false;
        sl << "-i" << videoPath;
        sl << "-q:a" << "0";
//        sl << "-filter:a" << "volume=4.0";
        sl << "-map" << "a";
        if(kbNeedPresets) sl << "-preset" << ksPresetName;
        sl << tempAudio;
        executeCommand(ffExecutable(), sl, &ok, true);
        sl.clear();

        if(ok)
        {
            QString oldOutput = m_strOutputFile;
            setOutputFile(tempVideo);
            addAudioOverlay(tempAudio);
            setOutputFile(oldOutput);
            videoOverlaySrc = tempVideo;
        }
    }

    sl << "-i" << videoOverlaySrc;
    sl << "-i" << videoPath;
    if(kbNeedChangeFPS) sl << "-r" << QString::number(knFPSValue);
    if(kbNeedChangeCRF) sl << "-crf" << QString::number(knCRFValue);
//    sl << "-filter_complex" << "[0:v][1:v] overlay=(W-w)/2:(H-h)/2 [out]";
    sl << "-filter_complex" << "[1:v][0:v]scale2ref[ua][b];[ua]setsar=1,format=yuva444p,colorchannelmixer=aa=0.5[u];[b][u]overlay=eof_action=pass[out]";
    sl << "-map" << "[out]";
    sl << "-map" << "0:a";
    if(kbNeedPresets) sl << "-preset" << ksPresetName;
    sl << m_strOutputFile;

    executeCommand(ffExecutable(), sl, nullptr, true);
    sl.clear();

    removeTempMedia();
}

//void FFmpegQT::processError(QProcess::ProcessError e)
//{
//    QMessageBox::information(nullptr,tr("ffmpeg"),tr("Error during the converting process : ")+QString::number(e));
//    //    qDebug() << "ffmpeg error:  " << e;
//}

qint64 FFmpegQT::getVideoDurationMs()
{
    if(!checkResFiles(false)) return -1;

    QStringList sl;

    sl << "-i" << m_strInputFile;
    sl << "-show_entries" << "format=duration";
    sl << "-v" << "quiet";
    sl << "-of" << "csv=p=0";
    if(kbNeedPresets) sl << "-preset" << ksPresetName;

    bool ok=false;
    qint64 duration=-1;
    QString res = executeCommand(ffProbeExe(), sl, &ok);

    if(ok)
       duration = static_cast<qint64>(res.toDouble(&ok)*1000.0);

    if(!ok) return -1;
    return duration;
}

QSize FFmpegQT::getVideoRectSize()
{
    if(!checkResFiles(false)) return QSize();

    QStringList sl;

    sl << "-v" << "error";
    sl << "-select_streams" << "v:0";
    sl << "-show_entries" << "stream=width,height";
    sl << "-of" << "csv=s=x:p=0";
    if(kbNeedPresets) sl << "-preset" << ksPresetName;
    sl << m_strInputFile;

    bool ok=false;
    QString res = executeCommand(ffProbeExe(), sl, &ok);

    if(ok)
    {
        QStringList sizeList = res.split("x");
        if(sizeList.count()==2)
        {
            int w = sizeList.at(0).toInt(&ok);
            int h = sizeList.at(1).toInt(&ok);

            if(ok)
                return QSize(w,h);
        }
    }

    return QSize();
}

bool FFmpegQT::isMediaContainAudio(const QString &mediaPath)
{
    if(!checkResFiles(false)) return false;

    QStringList sl;

    if(mediaPath.isEmpty())
        sl << "-i" << m_strInputFile;
    else
        sl << "-i" << mediaPath;
    sl << "-show_streams" << "-select_streams" << "a";
    sl << "-loglevel" << "error";
    if(kbNeedPresets) sl << "-preset" << ksPresetName;

    bool ok=false;
    QString res = executeCommand(ffProbeExe(), sl, &ok);

    return ok && !res.isEmpty();
}

void FFmpegQT::openInSystemPlayer(const QString &videoPath)
{
    QString path = (videoPath.isEmpty() ? m_strOutputFile : videoPath);
    QStringList arguments;
    m_playProcess->kill();
    arguments << path;
    if(path.isEmpty() || !QFile::exists(path))
        return;
    m_playProcess->start(ffPlayExe(), arguments);

    bool isOk = m_playProcess->waitForStarted();
    isOk = m_playProcess->waitForFinished(-1);
    QByteArray result = m_playProcess->readAllStandardOutput();
    if(!isOk)
    {
        QString errStr = m_playProcess->readAllStandardError();
        qDebug() << tr("Error during ffmpeg command. Code: ") << m_playProcess->exitCode() << tr(" . Description: ") << errStr;
    }

    //myProcess->deleteLater();
}

void FFmpegQT::slowDown(const QVariant &slowVal)
{
    slowDown(slowVal.toDouble());
}

void FFmpegQT::slowDownPart(const QVariant &slowVal, const QVariant &msBegin, const QVariant &msDuration)
{
    slowDownPart(slowVal.toDouble(), msBegin.toInt(), msDuration.toInt());
}

void FFmpegQT::addIntro(const QVariant &introPath)
{
    addIntro(introPath.toString());
}

void FFmpegQT::addOutro(const QVariant &outroPath)
{
    addOutro(outroPath.toString());
}

void FFmpegQT::changeResolution(const QVariant & newResolution)
{
    changeResolution(newResolution.toInt());
}

void FFmpegQT::addAudioOverlay(const QVariant & audioPath)
{
    addAudioOverlay(audioPath.toString());
}

void FFmpegQT::addImageOverlay(const QVariant & imagePath)
{
    addImageOverlay(imagePath.toString());
}

void FFmpegQT::addVideoOverlay(const QVariant & videoPath)
{
    addVideoOverlay(videoPath.toString());
}

void FFmpegQT::removeTempMedia()
{
    if(QFile::exists(tempVideo))
        QFile::remove(tempVideo);

    if(QFile::exists(tempAudio))
        QFile::remove(tempAudio);
}

void FFmpegQT::removeExtraTempMedia()
{
    if(QFile::exists(tempVideo2))
        QFile::remove(tempVideo2);

    if(QFile::exists(tempVideo3))
        QFile::remove(tempVideo3);

    if(QFile::exists(tempVideo4))
        QFile::remove(tempVideo4);

    if(QFile::exists(tempVideo5))
        QFile::remove(tempVideo5);
}

bool FFmpegQT::checkResFiles(bool removeOutput)
{
    if(m_strInputFile.isEmpty()) {
        QMessageBox::information(nullptr,tr("ffmpeg"),tr("Входной файл не указан"));
        return false;
    }
    else if(!QFile::exists(m_strInputFile))
    {
        QMessageBox::information(nullptr,tr("ffmpeg"),tr("Входной файл не существует"));
        return false;
    }

    if(m_strOutputFile.isEmpty()) {
        QMessageBox::information(nullptr,tr("ffmpeg"),tr("Выходной файл не указан"));
        return false;
    }
    else if(QFile::exists(m_strOutputFile))
    {
        if(removeOutput && !QFile::remove(m_strOutputFile))
            return false;
    }

    return true;
}

QString FFmpegQT::ffExecutable() const
{
    QString program = "ffmpeg.exe";

    if(!QFile::exists(program))
    {
        QMessageBox::information(nullptr,tr("ffmpeg"),tr("ffmpeg.exe файл не найден"));
        QCoreApplication::quit();
    }

    return program;
}

QString FFmpegQT::ffProbeExe() const
{
    QString program = "ffprobe.exe";

    if(!QFile::exists(program))
    {
        QMessageBox::information(nullptr,tr("ffmpeg"),tr("ffprobe.exe файл не найден"));
        QCoreApplication::quit();
    }

    return program;
}

QString FFmpegQT::ffPlayExe() const
{
    QString program = "ffplay.exe";

    if(!QFile::exists(program))
    {
        QMessageBox::information(nullptr,tr("ffmpeg"),tr("ffplay.exe файл не найден"));
        QCoreApplication::quit();
    }

    return program;
}

QString FFmpegQT::executeCommand(QString command, QStringList arguments, bool *ok, bool bNeedLogs)
{
    if(!(*m_nCancellation)) return "";

    bool *finished = new bool;
    int *exitCode = new int;
    *finished = false;
    QString string;
    QProcess *myProcess = new QProcess();
    myProcess->setProcessChannelMode(QProcess::MergedChannels);
//    QObject::connect(myProcess, &QProcess::errorOccurred, this, &FFmpegQT::processError);

    QMetaObject::Connection conn = QObject::connect(myProcess, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
    [=](int code, QProcess::ExitStatus)
    {
        *exitCode = code;
        *finished = true;
    });

    myProcess->start(command, arguments);
    bool isOk = myProcess->waitForStarted();
    if(!isOk)
    {
        if(ok) *ok = isOk;
        return "";
    }

    while((*m_nCancellation) && !(*finished)/*myProcess->waitForFinished(-1)*/)
    {
        QApplication::processEvents();
        qApp->thread()->msleep(100);
        if(bNeedLogs && m_logCB)
        {
            m_logCB(myProcess->readAllStandardOutput(), m_logCBargument);
        }
    }

    QByteArray result;
    if(!(*m_nCancellation))
    {
        QObject::disconnect(conn);
        myProcess->kill();
    }
    else
    {
        isOk &= myProcess->exitStatus()==QProcess::NormalExit;
        result = myProcess->readAllStandardOutput();

        if(bNeedLogs && m_logCB)
        {
            m_logCB(result, m_logCBargument);
        }

        if(!isOk || *exitCode==1)
        {
            QString errStr = myProcess->readAllStandardError();
            QMessageBox::information(nullptr,tr("ffmpeg"),tr("Ошибка во время команды ffmpeg: ")+arguments.join(" ")/*+tr(" . Описание: ")+errStr*/);
        }
    }

    myProcess->deleteLater();
    if(ok) *ok = isOk;
    return string.append(result);
}
