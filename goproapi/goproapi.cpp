#include "goproapi.h"

#include <QUdpSocket>
#include <QHostAddress>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QEventLoop>
#include <QTimer>
#include <QMessageBox>
#include <QFileDialog>
#include <QThread>

static const int kiMsTimeOut = 3000;

static void WakeOnLan(QString mac, QString ip)
{
    if(mac.isEmpty()) return;

    char MACAddr [6];
    char MagicPacket [102]; // Magic package for remote boot

    sscanf (mac.toLatin1().data(), "%2x%2x%2x%2x%2x%2x",
                    & MACAddr [0], & MACAddr [1], & MACAddr [2], & MACAddr [3], & MACAddr [4], & MACAddr [5]);

    // Set to hexadecimal before the magicpacket array 6 characters ff
    memset (MagicPacket, 0xff, 6);

    int packetsize = 6; // the beginning of the initial value is 6, do not wrong. I is because the effect of the initial value of the written as 0, it is too effortless.
    // Build MagicPacket.
    for (int i = 0; i <16; i++)
    {
        memcpy (MagicPacket + packetsize, MACAddr, 6);
        packetsize += 6;
    }

    QHostAddress FakeAddress;
    FakeAddress.setAddress (ip);

    QUdpSocket udpSocket;
    udpSocket.writeDatagram(MagicPacket, 102, FakeAddress, 9);
}

GoProApiQT::GoProApiQT(QObject *parent)
    : QObject(parent)
{
    FillHeroCommands();
    //WakeOnLan("F4DD9E72F6AC", GetIPAdress());
    //Connect(4000);
}

QString GoProApiQT::Get(GoProApiQT::HeroCommands heroCommands, int msTimeout, const QString &arg, bool showDialogs, const QString &fileName)
{
    QString command = m_mpHeroCommands[heroCommands];

    if (!arg.isEmpty())
    {
        command += arg;
    }

    QFile *file = nullptr;
    if(!fileName.isEmpty())
    {
        file = new QFile(fileName);
        if(!file->open(QIODevice::ReadWrite))
            return "";
    }

    QTimer timer;
    timer.setSingleShot(true);
    QNetworkRequest request;
    QEventLoop connection_loop;
    QNetworkAccessManager manager;

    connect(&manager, &QNetworkAccessManager::finished, &connection_loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, &connection_loop, &QEventLoop::quit);



    request.setUrl(GetHTTPIPAdress()+command);
    QNetworkReply* reply = manager.get(request);
    reply->setReadBufferSize(0);

    QObject::connect(reply, &QNetworkReply::readyRead, [&]{
        //this will be called every time a chunk of data is received
        if(file)
        {
            QByteArray data = reply->readAll();
            file->write(data);
        }
    });

    timer.start(msTimeout);
    connection_loop.exec();

    if(timer.isActive())
    {
        timer.stop();

        if(reply == nullptr)
        {
            if(showDialogs) QMessageBox::warning(nullptr, tr("Ошибка"), tr("Ошибка соединения"));
        }
        else if(reply->error())
        {
            if(showDialogs) QMessageBox::warning(nullptr, tr("Ошибка"), reply->errorString());
        }
        else
        {
            reply->setParent(nullptr);
            reply->deleteLater();

            if(file)
            {
                QByteArray data= reply->readAll();
                file->write(data);
                file->close();
            }

            return reply->readAll();
        }
    }
    else
    {
        if(showDialogs) QMessageBox::information(nullptr, tr("Тайм-аут сетевого соединения"), tr("Включите GoPro и подключитесь к Wi-Fi устройства"));
        disconnect(&manager, &QNetworkAccessManager::finished, &connection_loop, &QEventLoop::quit);
        reply->abort();
    }

    if(reply)
    {
        reply->setParent(nullptr);
        reply->deleteLater();
    }
    return "";
}

void GoProApiQT::FillHeroCommands()
{
    m_mpHeroCommands[HeroCommands::GpControl] = "/gp/gpControl/info";
    m_mpHeroCommands[HeroCommands::Shutter0] = "shutter?p=0";
    m_mpHeroCommands[HeroCommands::Shutter1] = "shutter?p=1";
    m_mpHeroCommands[HeroCommands::GpCommand] = "/gp/gpControl/command/";
    m_mpHeroCommands[HeroCommands::GpControlStatus] = "/gp/gpControl/status";
    m_mpHeroCommands[HeroCommands::DeleteLast] = "/gp/gpControl/command/storage/delete/last";
    m_mpHeroCommands[HeroCommands::GpMedialist] = ":8080/gp/gpMediaList";
}

bool GoProApiQT::CheckConnection()
{
    Connect(500, false);
    if(!isConnected())
    {
        if(m_sSessionMac!="")
        {
            WakeOnLan(m_sSessionMac, GetIPAdress());
            QThread::currentThread()->msleep(8000); //wait for turning on
            Connect(500, false);
            if(!isConnected()) {
                m_sSessionMac = "";
                QMessageBox::warning(nullptr, tr("Ошибка"), tr("Проверьте соединение с GoPro"));
                emit connectionChanged(false);
                return false;
            }
        }
    }

    return true;
}

void GoProApiQT::StartVideo()
{
    if(CheckConnection()) {
        QString param = m_mpHeroCommands[HeroCommands::Shutter1];
        Get(HeroCommands::GpCommand, kiMsTimeOut, param);
    }
}

void GoProApiQT::StopVideo()
{
    if(CheckConnection()) {
        QString param = m_mpHeroCommands[HeroCommands::Shutter0];
        Get(HeroCommands::GpCommand, kiMsTimeOut, param);
    }
}

bool GoProApiQT::LoadFromCamera(QString &outPath, const QString &destDir, bool needRemoveSrc)
{
    if(CheckConnection()) {
        QString dir = destDir;
        if(dir.isEmpty())
            dir = QFileDialog::getExistingDirectory(nullptr, ("Выбрать выходную папку"), QDir::currentPath());

        QString jsonString = Get(HeroCommands::GpMedialist, kiMsTimeOut);
        if (jsonString.isEmpty())
        {
            return false;
        }

        QJsonDocument jsonResponse = QJsonDocument::fromJson(jsonString.toUtf8());
        if(!jsonResponse.isNull())
        {
            QJsonObject jsonObject = jsonResponse.object();
            if(!jsonObject.isEmpty())
            {
                QJsonArray mediaArray = jsonObject["media"].toArray();
                if(mediaArray.isEmpty())
                    return false;

                QString d_Val;
                QString n_Val;

                foreach (const QJsonValue & media, mediaArray)
                {
                    d_Val = media.toObject().value("d").toString();
                    QJsonArray fsArray = media.toObject().value("fs").toArray();
                    if(fsArray.isEmpty())
                        return false;

                    foreach (const QJsonValue & fs, fsArray)
                    {
                        n_Val = fs.toObject().value("n").toString();
                    }
                }

                if(d_Val.isEmpty() || n_Val.isEmpty())
                    return false;

                QDir fileDir(dir+"/"+d_Val);
                if (!fileDir.exists()) {
                    fileDir.mkpath(".");
                }

                outPath = dir+"/"+n_Val;
                QString res = Get(HeroCommands::EmptyCommand, 120000, QString(":8080/videos/DCIM/%1/%2").arg(d_Val).arg(n_Val),
                                  true, outPath);

                if(needRemoveSrc)
                    Get(HeroCommands::DeleteLast, kiMsTimeOut);

                return true;
            }
        }
    }

    return false;
}

bool GoProApiQT::CameraIsBusy()
{
    QString text = Get(HeroCommands::GpControlStatus, kiMsTimeOut);

    if (text.isEmpty())
    {
        return false;
    }

    int num = text.indexOf("settings");
    if (num > 0 && text.mid(0, num).contains("\"8\":1"))
    {
        return true;
    }

    return false;
}

void GoProApiQT::DeleteLastMedia()
{
    if(CheckConnection()) {
        Get(HeroCommands::DeleteLast, kiMsTimeOut);
    }
}

bool GoProApiQT::Connect(int msTimeout, bool showDialogs)
{
    if(msTimeout==-1) msTimeout = kiMsTimeOut;
    QString strJson = Get(HeroCommands::GpControl, msTimeout, "", showDialogs);

    if (strJson.isEmpty())
    {        
        m_bConnected = false;
        emit connectionChanged(false);
        return false;
    }

    QJsonDocument jsonResponse = QJsonDocument::fromJson(strJson.toUtf8());
    if(!jsonResponse.isNull())
    {
        QJsonObject jsonObject = jsonResponse.object();
        if(!jsonObject.isEmpty())
        {
            QJsonObject info = jsonObject["info"].toObject();
            QString fwVersion = info["firmware_version"].toString();
            QString apmac = info["ap_mac"].toString();
            m_sSessionMac = apmac;

            if (fwVersion.contains("HD4") || fwVersion.contains("HD3.2") || fwVersion.contains("HD5")
                    || fwVersion.contains("HX") || fwVersion.contains("HD6"))
            {
                m_bConnected = true;
                emit connectionChanged(true);
                return true;
            }
            else if(!fwVersion.isEmpty())
                QMessageBox::warning(nullptr, tr("Ошибка"), tr("Версия этого GoPro не поддерживается"));
        }
    }

    m_bConnected = false;
    emit connectionChanged(false);
    return false;
}
