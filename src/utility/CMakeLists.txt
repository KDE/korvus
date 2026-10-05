add_library(utility STATIC httplogger.cpp chronoelapsedtimer.cpp qhauknam.cpp loghandler.cpp)
target_link_libraries(utility PRIVATE Qt::Core Qt::Network Qt::Qml)
