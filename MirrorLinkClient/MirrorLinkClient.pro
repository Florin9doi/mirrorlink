#-------------------------------------------------
#
# Project created by QtCreator 2015-11-03T23:15:22
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = MirrorLinkClient
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    ../CoreStack/remote_server.c \
    ../CoreStack/vnc_session.c \
    ../Platform/Linux/conn.c \
    ../Platform/Linux/http_client.c \
    ../Utils/buffer.c \
    ../Utils/str.c \
    mirrorlinkclient.cpp \
    threadmanager.cpp

HEADERS  += mainwindow.h \
    ../CoreStack/remote_server.h \
    ../CoreStack/vnc_session.h \
    ../Platform/conn.h \
    ../Platform/http_client.h \
    ../Utils/buffer.h \
    ../Utils/str.h \
    mirrorlinkclient.h \
    threadmanager.h

FORMS    += mainwindow.ui

unix:!macx: LIBS += -L/usr/local/libxml2/lib/ -lxml2

INCLUDEPATH += /usr/local/libxml2/include/libxml2 /usr/include/libxml2
DEPENDPATH += /usr/local/libxml2/include/libxml2
