#ifndef FFMPEGQT_H
#define FFMPEGQT_H

#include <QObject>
#include <QProcess>

typedef void (*logsCallBack)(QString logInfo, void *arg);

class FFmpegQT : public QObject
{
    Q_OBJECT
public:
    explicit FFmpegQT(QObject *parent = nullptr);
    ~FFmpegQT();

    void setInputFile(const QString &inFile); //use these methods before converting
    void setOutputFile(const QString &outFile); //use these methods before converting
    void setLogsCallback(logsCallBack logCB, void *arg) { m_logCB = logCB; m_logCBargument = arg; } // set this to output ffmpeg process logs
    void setCancellationVar(volatile bool *nCancellation) { m_nCancellation = nCancellation; } //set this to break ffmpeg command performing

    void setConvertionQuality(int nValue);
    void checkAndInsertSilence(const QString &path);

    QString getInputFile() const { return m_strInputFile; }
    QString getOutputFile() const { return m_strOutputFile; }

    qint64 getVideoDurationMs();
    QSize getVideoRectSize();
    bool isMediaContainAudio(const QString &mediaPath="");

    void openInSystemPlayer(const QString &videoPath="");

    enum eFFMPEGCommand {
        SlowDown,
        SlowDownPart,
        Crop1x1,
        Reverse,
        VideoOverlay,
        ImageOverlay,
        AudioOverlay,
        AddIntro,
        AddOutro,        
        Compress
    };

public slots:
    void slowDown(const QVariant &slowVal);
    void slowDownPart(const QVariant &slowVal, const QVariant &msBegin, const QVariant &msDuration);
    void addIntro(const QVariant & introPath);
    void addOutro(const QVariant & outroPath);
    void changeResolution(const QVariant & newResolution); //newResolution: 1 - hd, 2 - fullhd
    void addAudioOverlay(const QVariant & audioPath);
    void addImageOverlay(const QVariant & imagePath);
    void addVideoOverlay(const QVariant & videoPath);

    void slowDown(double slowVal);
    void slowDownPart(double slowVal, int msBegin, int msDuration);
    void reverse();
    void crop1x1();
    void addIntro(const QString &introPath);
    void addOutro(const QString &outroPath);
    void changeResolution(int newResolution=1); //newResolution: 1 - hd, 2 - fullhd
    void addAudioOverlay(const QString &audioPath);
    void addImageOverlay(const QString &imagePath);
    void addVideoOverlay(const QString &videoPath);

protected slots:
//    void processError(QProcess::ProcessError e);

protected:
    void removeTempMedia();
    void removeExtraTempMedia();
    bool checkResFiles(bool removeOutput=false);
    QString ffExecutable() const;
    QString ffProbeExe() const;
    QString ffPlayExe() const;
    QString executeCommand(QString command, QStringList arguments, bool *ok=nullptr, bool bNeedLogs=false);

    int m_nConvertionQuality;
    volatile bool *m_nCancellation;
    logsCallBack m_logCB;
    void *m_logCBargument;
    QProcess *m_playProcess;
    QString m_strInputFile;
    QString m_strOutputFile;
};

#endif // FFMPEGQT_H
