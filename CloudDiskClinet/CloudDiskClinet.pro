QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    common/aeskeyinstance.cpp \
    common/common.cpp \
    common/downdask.cpp \
    common/downloadlayout.cpp \
    common/logininfoinstance.cpp \
    common/qaesencryption.cpp \
    common/uploadlayout.cpp \
    common/uploadtask.cpp \
    login.cpp \
    main.cpp \
    mainwindow.cpp \
    myfile.cpp \
    ranking.cpp \
    selfwidget/buttongroup.cpp \
    selfwidget/dataprogress.cpp \
    selfwidget/fileproperty.cpp \
    selfwidget/mymenu.cpp \
    selfwidget/titlewidget.cpp \
    sharefile.cpp \
    transform.cpp

HEADERS += \
    aesni/aesni-enc-cbc.h \
    aesni/aesni-enc-ecb.h \
    aesni/aesni-key-exp.h \
    common/aeskeyinstance.h \
    common/common.h \
    common/downdask.h \
    common/downloadlayout.h \
    common/logininfoinstance.h \
    common/qaesencryption.h \
    common/uploadlayout.h \
    common/uploadtask.h \
    login.h \
    mainwindow.h \
    myfile.h \
    ranking.h \
    selfwidget/buttongroup.h \
    selfwidget/dataprogress.h \
    selfwidget/fileproperty.h \
    selfwidget/mymenu.h \
    selfwidget/titlewidget.h \
    sharefile.h \
    transform.h

FORMS += \
    login.ui \
    mainwindow.ui \
    myfile.ui \
    ranking.ui \
    selfwidget/buttongroup.ui \
    selfwidget/dataprogress.ui \
    selfwidget/fileproperty.ui \
    selfwidget/titlewidget.ui \
    sharefile.ui \
    transform.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc
