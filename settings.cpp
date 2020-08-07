#include <QFile>
#include "settings.h"

Settings &Settings::instance()
{
    static Settings instance;
    return instance;
}

Settings::Settings()
{
    {
        QFile settingsFile(settingsFileName);
        if (!settingsFile.exists())
        {
            settingsFile.open(QFile::WriteOnly);
            isValid = false;
        }
    }

    m_settings = new QSettings(settingsFileName, QSettings::IniFormat);
    m_settings->setIniCodec("UTF-8");

    bool ok=true;

    serialPortInfoLBIndex = m_settings->value(settings::serialPortInfoLBIndex).toInt(&ok); isValid &= ok;
    timeSpinValue = m_settings->value(settings::timeSpinValue).toInt(&ok); isValid &= ok;
    speedSpinValue = m_settings->value(settings::speedSpinValue).toInt(&ok); isValid &= ok;
    cbSlowDownValue = m_settings->value(settings::cbSlowDownValue).toBool();
    slowDownValue = m_settings->value(settings::slowDownValue).toInt(&ok); isValid &= ok;
    slowDownBegin = m_settings->value(settings::slowDownBegin).toDouble(&ok); isValid &= ok;
    slowDownEnd = m_settings->value(settings::slowDownEnd).toDouble(&ok); isValid &= ok;
    cbNoRecordingPerform = m_settings->value(settings::cbNoRecordingPerform).toBool();
    anotherVideoPath = m_settings->value(settings::anotherVideoPath).toString();
    cbRecordTime = m_settings->value(settings::cbRecordTime).toBool();
    recordTimeValue = m_settings->value(settings::recordTimeValue).toDouble(&ok); isValid &= ok;
    originalVideoPath = m_settings->value(settings::originalVideoPath).toString();
    savingVideoPath = m_settings->value(settings::savingVideoPath).toString();
    audioOverlay = m_settings->value(settings::audioOverlay).toBool();
    audioOverlayPath = m_settings->value(settings::audioOverlayPath).toString();
    imageOverlay = m_settings->value(settings::imageOverlay).toBool();
    imageOverlayPath = m_settings->value(settings::imageOverlayPath).toString();
    videoOverlay = m_settings->value(settings::videoOverlay).toBool();
    videoOverlayPath = m_settings->value(settings::videoOverlayPath).toString();
    introVideo = m_settings->value(settings::introVideo).toBool();
    introVideoPath = m_settings->value(settings::introVideoPath).toString();
    outroVideo = m_settings->value(settings::outroVideo).toBool();
    outroVideoPath = m_settings->value(settings::outroVideoPath).toString();
    removeFromCamera = m_settings->value(settings::removeFromCamera).toBool();
    cropVideo1x1 = m_settings->value(settings::cropVideo1x1).toBool();
    reverseVideo = m_settings->value(settings::reverseVideo).toBool();
    comboCompressionIndex = m_settings->value(settings::comboCompressionIndex).toInt(&ok); isValid &= ok;
}

Settings::~Settings()
{
    m_settings->setValue(settings::serialPortInfoLBIndex, serialPortInfoLBIndex);
    m_settings->setValue(settings::timeSpinValue, timeSpinValue);
    m_settings->setValue(settings::speedSpinValue, speedSpinValue);
    m_settings->setValue(settings::cbSlowDownValue, cbSlowDownValue);
    m_settings->setValue(settings::slowDownValue, slowDownValue);
    m_settings->setValue(settings::slowDownBegin, slowDownBegin);
    m_settings->setValue(settings::slowDownEnd, slowDownEnd);
    m_settings->setValue(settings::cbNoRecordingPerform, cbNoRecordingPerform);
    m_settings->setValue(settings::anotherVideoPath, anotherVideoPath);
    m_settings->setValue(settings::cbRecordTime, cbRecordTime);
    m_settings->setValue(settings::recordTimeValue, recordTimeValue);
    m_settings->setValue(settings::originalVideoPath, originalVideoPath);
    m_settings->setValue(settings::savingVideoPath, savingVideoPath);
    m_settings->setValue(settings::audioOverlay, audioOverlay);
    m_settings->setValue(settings::audioOverlayPath, audioOverlayPath);
    m_settings->setValue(settings::imageOverlay, imageOverlay);
    m_settings->setValue(settings::imageOverlayPath, imageOverlayPath);
    m_settings->setValue(settings::videoOverlay, videoOverlay);
    m_settings->setValue(settings::videoOverlayPath, videoOverlayPath);
    m_settings->setValue(settings::introVideo, introVideo);
    m_settings->setValue(settings::introVideoPath, introVideoPath);
    m_settings->setValue(settings::outroVideo, outroVideo);
    m_settings->setValue(settings::outroVideoPath, outroVideoPath);
    m_settings->setValue(settings::removeFromCamera, removeFromCamera);
    m_settings->setValue(settings::cropVideo1x1, cropVideo1x1);
    m_settings->setValue(settings::reverseVideo, reverseVideo);
    m_settings->setValue(settings::comboCompressionIndex, comboCompressionIndex);

    m_settings->sync();
    delete m_settings;
}
