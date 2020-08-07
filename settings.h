#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QSettings>

const QString settingsFileName("config.ini");

namespace settings {
    const QString serialPortInfoLBIndex("serialPortInfoLBIndex");
    const QString timeSpinValue("timeSpinValue");
    const QString speedSpinValue("speedSpinValue");
    const QString cbSlowDownValue("cbSlowDownValue");
    const QString slowDownValue("slowDownValue");
    const QString slowDownBegin("slowDownBegin");
    const QString slowDownEnd("slowDownEnd");
    const QString cbNoRecordingPerform("cbNoRecordingPerform");
    const QString anotherVideoPath("anotherVideoPath");
    const QString cbRecordTime("cbRecordTime");
    const QString recordTimeValue("recordTimeValue");
    const QString originalVideoPath("originalVideoPath");
    const QString savingVideoPath("savingVideoPath");
    const QString audioOverlay("audioOverlay");
    const QString audioOverlayPath("audioOverlayPath");
    const QString imageOverlay("imageOverlay");
    const QString imageOverlayPath("imageOverlayPath");
    const QString videoOverlay("videoOverlay");
    const QString videoOverlayPath("videoOverlayPath");
    const QString introVideo("introVideo");
    const QString introVideoPath("introVideoPath");
    const QString outroVideo("outroVideo");
    const QString outroVideoPath("outroVideoPath");
    const QString removeFromCamera("removeFromCamera");
    const QString cropVideo1x1("cropVideo1x1");
    const QString reverseVideo("reverseVideo");
    const QString comboCompressionIndex("comboCompressionIndex");
}


class Settings : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Settings)
public:
    static Settings &instance();
    bool isValid=true;

     int serialPortInfoLBIndex;
     int timeSpinValue;
     int speedSpinValue;
     bool cbSlowDownValue;
     int slowDownValue;
     double slowDownBegin;
     double slowDownEnd;
     bool cbNoRecordingPerform;
     QString anotherVideoPath;
     bool cbRecordTime;
     double recordTimeValue;
     QString originalVideoPath;
     QString savingVideoPath;
     bool audioOverlay;
     QString audioOverlayPath;
     bool imageOverlay;
     QString imageOverlayPath;
     bool videoOverlay;
     QString videoOverlayPath;
     bool introVideo;
     QString introVideoPath;
     bool outroVideo;
     QString outroVideoPath;
     bool removeFromCamera;
     bool cropVideo1x1;
     bool reverseVideo;
     int comboCompressionIndex;

private:
    Settings();
    ~Settings();

    QSettings *m_settings;
};

#define SETTINGS Settings::instance()

#endif // SETTINGS_H
