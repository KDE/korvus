cmake_minimum_required(VERSION 4.0)
project(Korvus LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# disable pointless warning in KDECMakeSettings
set(APPLE_SUPPRESS_X11_WARNING ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

include(FeatureSummary)
if(FALSE)

find_package(ECM 6.0.0 REQUIRED NO_MODULE)

set_package_properties(ECM PROPERTIES TYPE REQUIRED DESCRIPTION "Extra CMake Modules." URL "https://projects.kde.org/projects/kdesupport/extra-cmake-modules")

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${ECM_MODULE_PATH} ${CMAKE_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(ECMQmlModule)

#include(KDECompilerSettings NO_POLICY_SCOPE)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Quick QuickWidgets QuickControls2 OpenApiCommon Positioning Location)
if (NOT IOS)
    find_package(Qt6 REQUIRED COMPONENTS OpenApiTools)
endif()
qt_standard_project_setup(REQUIRES 6.11)


add_subdirectory(src)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES INCLUDE_QUIET_PACKAGES)
