#-------------------------------------------------
#
# Project created by QtCreator 2018-11-23T14:06:05
#
#-------------------------------------------------

QT       += core gui charts xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += c++11
TARGET = V300_NLC_TOOL
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
        mainwindow.cpp \
    aboutdialog.cpp \
    rawinfodialog.cpp \
    nlccollection.cpp \
    imglabel.cpp \
    chartdialog.cpp \
    chartview.cpp \
    rightshiftdialog.cpp \
    chart.cpp \
    blcdialog.cpp \
    calcblcthread.cpp \
    calcblcprogressdlg.cpp \
    blcxmlhighlight.cpp

HEADERS += \
        inc/mainwindow.h \
    inc/aboutdialog.h \
    inc/rawinfodialog.h \
    inc/nlccollection.h \
    inc/imglabel.h \
    inc/chartdialog.h \
    inc/chartview.h \
    inc/rightshiftdialog.h \
    inc/chart.h \
    inc/blcdialog.h \
    inc/calcblcthread.h \
    inc/calcblcprogressdlg.h \
    inc/blcxmlhighlight.h

FORMS += \
        ui\mainwindow.ui \
    ui\aboutdialog.ui \
    ui\rawinfodialog.ui \
    ui\rightshiftdialog.ui \
    ui\calcblcprogressdlg.ui

INCLUDEPATH +=  C:\python3\include \
        C:\python3\Lib\site-packages\numpy\core\include
        #E:\yqzeng\opencv3\build_x64\include \

LIBS += C:\python3\libs\python35.lib
        #E:\yqzeng\opencv3\build_x64\x64\vc15\lib\opencv_core341d.lib \
        #E:\yqzeng\opencv3\build_x64\x64\vc15\lib\opencv_imgproc341d.lib \


#INCLUDEPATH += D:\opencv3-4-1\build\include \
#        C:\python36\include \
#        C:\python36\Lib\site-packages\numpy\core\include
#
#LIBS += C:\python36\libs\python36.lib \
#        D:\opencv3-4-1\build\x64\vc14\lib\opencv_core343.lib \
#        D:\opencv3-4-1\build\x64\vc14\lib\opencv_imgproc343.lib



RESOURCES += \
    resource\hisi_logo.qrc

RC_FILE = resource\icon.rc
