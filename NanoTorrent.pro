QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11
#PKGCONFIG += openssl
LIBS += -lssl -lcrypto

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
INCLUDEPATH += \
        $$PWD/core \
        $$PWD/core/bencode

SOURCES += \
    addtorrentdialog.cpp \
    clientinfodialog.cpp \
    newtorrentdialog.cpp \
    core/bencode/bencode.c \
    core/client.c \
    main.cpp \
    nt_gui.cpp \
    torrentmodel.cpp

HEADERS += \
    clientinfodialog.h \
    nt_gui.h \
    addtorrentdialog.h \
    newtorrentdialog.h \
    core/client.h \
    core/bencode/bencode.h \
    core/bencode/list.h \
    torrentmodel.h

FORMS += \
    addtorrentdialog.ui \
    clientinfodialog.ui \
    newtorrentdialog.ui \
    nt_gui.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    icons.qrc
