TEMPLATE = app
QT += core gui multimedia network texttospeech widgets
TARGET = Ai
CONFIG += c++17

# Android-specific configurations
android {
    message("Android enabled")
    QT += core-private

    # OpenSSL configuration
    ANDROID_OPENSSL_ROOT = C:\Users\MarenCompEng\Desktop\Setup\openssl\ssl_3

    # Explicitly set the target architecture to armeabi-v7a
    ANDROID_ABIS = armeabi-v7a
    INCLUDEPATH += $$ANDROID_OPENSSL_ROOT/include

# Specify the full names of the OpenSSL libraries
   ANDROID_EXTRA_LIBS += \
       $$ANDROID_OPENSSL_ROOT/armeabi-v7a/libcrypto_3.so \
       $$ANDROID_OPENSSL_ROOT/armeabi-v7a/libssl_3.so

    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
    DISTFILES += android/AndroidManifest.xml

    ANDROID_PLATFORM = android-33
    ANDROID_NDK_PLATFORM = android-21

    # Ensure we're building only for armeabi-v7a
    contains(ANDROID_TARGET_ARCH, armeabi-v7a) {
        message("Building for armeabi-v7a")
    } else {
        error("Unsupported Android architecture. Please use armeabi-v7a.")
    }
}

# macOS-specific configurations
macx {
    message("macOS enabled")
    RC_ICONS = $$PWD/icons/ai.ico
    QMAKE_INFO_PLIST = $$PWD/Info.plist.in
    LIBS += -framework AudioToolbox
}

# iOS-specific configurations
ios {
    message("iOS enabled")
    QMAKE_INFO_PLIST = ios/Info.plist
    QMAKE_ASSET_CATALOGS = $$PWD/ios/Assets.xcassets
    QMAKE_ASSET_CATALOGS_APP_ICON = "AppIcon"
    LIBS += -framework AVFoundation
    LIBS += -framework AudioToolbox
}

# Windows-specific configurations
win32 {
    message("Windows enabled")
    INCLUDEPATH += $$PWD
}

# Linux-specific configurations
unix:!mac:!android {
    message("Linux enabled")
}

# Header files
HEADERS = \
    ai.h \
    audiolevel.h \
    constants.h \
    gpt3client.h \
    translateclient.h

# Source files
SOURCES = \
    ai.cpp \
    main.cpp \
    audiolevel.cpp

# Forms
FORMS += \
    ai.ui

# Default rules for deployment
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# Resources
RESOURCES += \
    resources.qrc

# Include shared project file
include(./shared/shared.pri)

# Additional comments (kept from original file)
#ffmpeg -list_devices true -f dshow -i null
#ffmpeg -f dshow -t 30 -i "audio=Microphone (2- USB PnP Audio Device(EEPROM))" record.flac

# For non-Android platforms, keep the general SSL library linking
!android {
    LIBS += -lssl -lcrypto
}
