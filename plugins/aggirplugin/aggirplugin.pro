TEMPLATE = lib
TARGET = Gir
PACKAGE_VERSION = 0.0.2
DEFINES += AGGIR_LIBRARY

BUILD_PATH_POSTFIXE = FreeMedForms

include(../fmf_plugins.pri)
include( aggirplugin_dependencies.pri )
HEADERS = \
    aggirplugin.h \
    aggir_exporter.h \
    girwidget.h

SOURCES = \
    aggirplugin.cpp \
    girwidget.cpp

OTHER_FILES = Gir.pluginspec

FORMS += girwidget.ui
