#-------------------------------------------------
#
# Project created by QtCreator 2018-07-19T00:55:55
#
#-------------------------------------------------

QT       += core gui serialport network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

TARGET = carousel_app
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        MainWindow.cpp \
    CarouselSerial.cpp \
    goproapi/goproapi.cpp \
    CarouselSerial.cpp \
    main.cpp \
    MainWindow.cpp \
    ffmpegqt.cpp \
    spoiler.cpp \
    videoeffects.cpp \
    logswindow.cpp \
    settings.cpp

HEADERS += \
        MainWindow.h \
    CarouselSerial.h \
    goproapi/goproapi.h \
    CarouselSerial.h \
    MainWindow.h \
    ffmpegqt.h \
    spoiler.h \
    videoeffects.h \
    logswindow.h \
    settings.h

FORMS += \
        MainWindow.ui \
    videoeffects.ui \
    logswindow.ui

RESOURCES += \
    resources.qrc

win32:RC_FILE = app.rc
