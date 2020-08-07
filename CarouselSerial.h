#ifndef CAROUSELSERIAL_H
#define CAROUSELSERIAL_H

#include <QObject>
#include <QSerialPort>

class CarouselSerial : public QObject
{
    Q_OBJECT
public:
    explicit CarouselSerial(QObject *parent = nullptr);

    enum MotorStatus {
        STOPPED_MOTOR_STATUS = 0,
        INIT_START_MOTOR_STATUS = 1,
        SMOOTH_START_MOTOR_STATUS = 2,
        ROTATED_MOTOR_STATUS = 3,
        INIT_STOP_MOTOR_STATUS = 4,
        SMOOTH_STOP_MOTOR_STATUS = 5,
        UNKNOWN_MOTOR_STATUS = 255
    };

signals:
    void serialPortOpenned();
    void serialPortClosed();
    void motosStatusChanged(MotorStatus status);

public slots:
    bool openSerialPort(const QString &portName);
    void closeSerialPort();

    bool testCommand();
    void start();
    void stop();

    bool setSpeed(int value);
    bool setTime(int value);

    int getSpeed();
    int getTime();

    MotorStatus getStatus();

private:
    void readData();
    void handleError(QSerialPort::SerialPortError error);

    QSerialPort *serialPort_ = nullptr;

    QByteArray data_;

    QByteArray sendCommad(QByteArray package);

    bool checksum(const QByteArray &package);
    bool isOkResponse(const QByteArray &package);
    void parsePackage(const QByteArray &package);
};

#endif // CAROUSELSERIAL_H
