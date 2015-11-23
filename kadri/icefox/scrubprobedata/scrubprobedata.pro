TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .
QT = core
CONFIG -= app_bundle
CONFIG += release
#CONFIG += debug
CONFIG += console
# Input

include(../src/netflixresultsgenerator.pri)

SOURCES += main.cpp

DESTDIR = ./
