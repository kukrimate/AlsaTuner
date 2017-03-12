QT       += core widgets

TARGET = AlsaTuner
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += main.cpp\
        alsatunergui.cpp \
    alsaapi.cpp

HEADERS  += alsatunergui.h \
    alsaapi.h

FORMS    += alsatunergui.ui
