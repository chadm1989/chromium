TEMPLATE = app
TARGET = tst_qwebhistory
include(../../../../WebKit.pri)
SOURCES  += tst_qwebhistory.cpp
RESOURCES  += tst_qwebhistory.qrc
QT += testlib network
QMAKE_RPATHDIR = $$OUTPUT_DIR/lib $$QMAKE_RPATHDIR

symbian:TARGET.UID3 = 0x200267C4
