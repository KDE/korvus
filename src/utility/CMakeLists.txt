add_library(utility STATIC httplogger.cpp chronoelapsedtimer.cpp qhauknam.cpp loghandler.cpp clipboardwatcher.cpp)
target_link_libraries(utility PRIVATE Qt::Gui Qt::Core Qt::Network Qt::Qml)
qt_add_qml_module(utility
        URI org.kde.hauk.utility
        VERSION 1.0
)
