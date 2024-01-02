TEMPLATE = app

QT += core gui multimedia network texttospeech
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Ai
CONFIG += c++14

win32:INCLUDEPATH += $$PWD

HEADERS = \
    ai.h \
    audiolevel.h \
    constants.h \
    gpt3client.h

SOURCES = \
    ai.cpp \
    main.cpp \
    audiolevel.cpp

FORMS += \
    ai.ui


macx{
    message("macx enabled")
    RC_ICONS = $$PWD/icons/ai.ico
#    INCLUDEPATH += /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/Frameworks/AudioToolbox.framework/Headers
    LIBS += -framework AudioToolbox

#    QMAKE_INFO_PLIST = $$PWD/Info.plist.in
}

ios {
    message("ios enabled")
    QMAKE_INFO_PLIST = ios/Info.plist
    QMAKE_ASSET_CATALOGS = $$PWD/ios/Assets.xcassets
    QMAKE_ASSET_CATALOGS_APP_ICON = "AppIcon"    
    LIBS += -framework AVFoundation
    LIBS += -framework AudioToolbox
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

QT+=widgets
include(./shared/shared.pri)

#ffmpeg -list_devices true -f dshow -i null
#ffmpeg -f dshow -t 30 -i "audio=Microphone (2- USB PnP Audio Device(EEPROM))" record.flac
