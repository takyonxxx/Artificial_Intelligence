QT += core gui multimedia
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TARGET = Ai
CONFIG += c++17

win32:INCLUDEPATH += $$PWD

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    audiolevel.cpp \
    main.cpp \
    ai.cpp

HEADERS += \
    ai.h \
    audiolevel.h \
    constants.h

FORMS += \
    ai.ui

macx{
    message("macx enabled")
    RC_ICONS = $$PWD/icons/ai.ico
}

ios {
    message("ios enabled")
    QMAKE_INFO_PLIST = ios/Info.plist
    QMAKE_ASSET_CATALOGS = $$PWD/ios/Assets.xcassets
    QMAKE_ASSET_CATALOGS_APP_ICON = "AppIcon"
}

win32{
    message("Win32 enabled")
}

unix!mac{
    message("linux enabled")
}

android{
    message("android enabled")
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc
