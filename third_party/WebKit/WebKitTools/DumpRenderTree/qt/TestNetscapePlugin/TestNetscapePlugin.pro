TEMPLATE = lib
TARGET = TestNetscapePlugin

VPATH = ../../gtk/TestNetscapePlugin ../../TestNetscapePlugIn.subproj
include(../../../../WebKit.pri)

DESTDIR = $$OUTPUT_DIR/lib/plugins

mac {
    CONFIG += plugin
    CONFIG += plugin_bundle
    QMAKE_INFO_PLIST = ../../TestNetscapePlugIn.subproj/Info.plist
    QMAKE_PLUGIN_BUNDLE_NAME = $$TARGET
    QMAKE_BUNDLE_LOCATION += "Contents/MacOS"

    debug_and_release:CONFIG(debug, debug|release) {
        TARGET = $$qtLibraryTarget($$TARGET)
    }
}

INCLUDEPATH += ../../../../JavaScriptCore \
               ../../gtk/TestNetscapePlugin/ForwardingHeaders \
               ../../gtk/TestNetscapePlugin/ForwardingHeaders/WebKit \
               ../../../../WebCore \
               ../../../../WebCore/bridge \
               ../../TestNetscapePlugIn.subproj

SOURCES = PluginObject.cpp \
          TestObject.cpp

mac {
    SOURCES += ../../TestNetscapePlugIn.subproj/main.cpp
} else {
    SOURCES += ../../gtk/TestNetscapePlugin/TestNetscapePlugin.cpp
}
