TARGET = %PluginName%
TEMPLATE = lib

DEFINES += %PluginName:u%_LIBRARY
BUILD_PATH_POSTFIXE = FreeMedForms

include(../fmf_plugins.pri)
include(%PluginName:l%_dependencies.pri)

HEADERS += \
    %PluginName:l%plugin.h\
    %PluginName:l%_exporter.h\
    %PluginName:l%constants.h
        
SOURCES += \
    %PluginName:l%plugin.cpp

OTHER_FILES = %PluginName%.pluginspec

equals(TEST, 1) {
    SOURCES += \
        tests/test_%PluginName:l%plugin.cpp
}

#include translations
TRANSLATION_NAME = %PluginName:l%
include($${SOURCES_ROOT_PATH}/buildspecs/translations.pri)
