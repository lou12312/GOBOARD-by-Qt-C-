QT       += core gui
QT += widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = GoGame
TEMPLATE = app

SOURCES += \
    main.cpp \
    goboard.cpp

HEADERS += \
    goboard.h

CONFIG += c++11
