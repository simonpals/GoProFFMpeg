#include "CarouselSerial.h"

#include <QDebug>
#include <QMessageBox>
#include <QThread>
#include <QApplication>

CarouselSerial::CarouselSerial(QObject *parent)
    : QObject(parent),
      serialPort_(new QSerialPort(this))
{
    connect(serialPort_, &QSerialPort::errorOccurred, this, &CarouselSerial::handleError);
    connect(serialPort_, &QSerialPort::readyRead, this, &CarouselSerial::readData);
}

bool CarouselSerial::openSerialPort(const QString &portName)
{
    serialPort_->setPortName(portName);
    serialPort_->setBaudRate(QSerialPort::Baud57600);
    /*serialPort_->setDataBits(QSerialPort::Data8);
    serialPort_->setParity(QSerialPort::NoParity);
    serialPort_->setStopBits(QSerialPort::OneStop);
    serialPort_->setFlowControl(QSerialPort::NoFlowControl);*/

    if (serialPort_->open(QIODevice::ReadWrite))
    {
        emit serialPortOpenned();
        return true;
    }
    else
    {
        emit serialPortClosed();
        QMessageBox::critical(nullptr, tr("Error"), serialPort_->errorString());
        return false;
    }
}

void CarouselSerial::closeSerialPort()
{
    if (serialPort_->isOpen())
        serialPort_->close();

    emit serialPortClosed();
}

bool CarouselSerial::testCommand()
{
    QByteArray command = "\xAB\xFF\xAA";

    QByteArray response = sendCommad(command);
    return isOkResponse(response);
}

void CarouselSerial::start()
{
    static const QByteArray command = "\xBC\xFF\xBB";
    serialPort_->write(command);
}

void CarouselSerial::stop()
{
    static const QByteArray command = "\xBE\xFF\xBD";
    serialPort_->write(command);
}

bool CarouselSerial::setSpeed(int value)
{
    QByteArray command = "\xAD";
    command.append((uint8_t)value);
    command.append((uint8_t)(command[0] + command[1]));

    QByteArray response = sendCommad(command);
    return isOkResponse(response);
}

bool CarouselSerial::setTime(int value)
{
    QByteArray command = "\xAF";
    command.append((uint8_t)value);
    command.append((uint8_t)(command[0] + command[1]));

    QByteArray response = sendCommad(command);
    return isOkResponse(response);
}

int CarouselSerial::getSpeed()
{
    QByteArray command = "\xCA\xFF\xC9";
    QByteArray response = sendCommad(command);

    if (checksum(response) && response[1] == '\xCA')
    {
        return response[2];
    }
    else
    {
        return -1;
    }
}

int CarouselSerial::getTime()
{
    QByteArray command = "\xCD\xFF\xCC";
    QByteArray response = sendCommad(command);

    if (checksum(response) && response[1] == '\xCD')
    {
        return response[2];
    }
    else
    {
        return -1;
    }
}

CarouselSerial::MotorStatus CarouselSerial::getStatus()
{
    QByteArray command = "\xDA\xFF\xD9";
    QByteArray response = sendCommad(command);

    if (checksum(response) && response[1] == '\xDA')
    {
        return (MotorStatus)((uint8_t)response[2]);
    }
    else
    {
        return UNKNOWN_MOTOR_STATUS;
    }
}

void CarouselSerial::readData()
{
    const QByteArray data = serialPort_->readAll();

    //qDebug() << QString(data);

    data_ += data;

    if (data_.size() < 4)
        return;

    QByteArray package = data_.mid(0, 4);
    data_ = data_.remove(0, 4);
    while (!package.isEmpty())
    {
        if (checksum(package))
        {
            parsePackage(package);
        }

        package = data_.mid(0, 4);
        data_ = data_.remove(0, 4);
    }

    data_.clear();
}

void CarouselSerial::handleError(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError)
    {
        //QMessageBox::critical(nullptr, tr("Critical Error"), serialPort_->errorString());
        qWarning() << serialPort_->errorString();
        closeSerialPort();
    }
    else
    {
        qWarning() << serialPort_->errorString();
    }
}

QByteArray CarouselSerial::sendCommad(QByteArray package)
{
    if (!serialPort_->isOpen())
    {
        emit serialPortClosed();
        return QByteArray();
    }

    disconnect(serialPort_, &QSerialPort::readyRead, this, &CarouselSerial::readData);

    serialPort_->write(package);
    serialPort_->flush();
    serialPort_->waitForBytesWritten(1000);

    if (!serialPort_->isOpen())
    {
        emit serialPortClosed();
        connect(serialPort_, &QSerialPort::readyRead, this, &CarouselSerial::readData);
        return QByteArray();
    }

    serialPort_->waitForReadyRead(1000);

    QByteArray data;
    int countAttempt = 0;

    while (data.size() < 4 && countAttempt < 10) {

        data += serialPort_->readAll();
        qApp->processEvents();
        QThread::msleep(200);
        countAttempt++;
    }

    data = data.mid(0, 4);

    connect(serialPort_, &QSerialPort::readyRead, this, &CarouselSerial::readData);

    return data;
}

bool CarouselSerial::checksum(const QByteArray &package)
{
    if (package.size() != 4)
        return false;

    return ((uint8_t)(package[0] + package[1] + package[2])) == ((uint8_t)package[3]);
}

bool CarouselSerial::isOkResponse(const QByteArray &package)
{
    return package == "\xAB\xFF\xFF\xA9";
}

void CarouselSerial::parsePackage(const QByteArray &package)
{
    if (package[0] == '\xAB' && package[1] == '\xDA')
    {
        MotorStatus ms = (MotorStatus)((uint8_t)package[2]);
        emit motosStatusChanged(ms);
    }
}
