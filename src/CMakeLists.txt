
add_subdirectory(utility)

add_library(qhauk_client STATIC)
if (TARGET Qt6::QtOpenAPIGeneratorJar)
    file(REMOVE_RECURSE ${CMAKE_CURRENT_SOURCE_DIR}/api/generated)
    qt_add_openapi_client(qhauk_client
        SPEC_FILE
            ${CMAKE_CURRENT_SOURCE_DIR}/api/hauk.yaml
        OUTPUT_DIRECTORY
            ${CMAKE_CURRENT_BINARY_DIR}/api/generated
        ADDITIONAL_PROPERTIES
            cppNamespace=Hauk
    )
    # we currently can't use the generator on ios, so we have to copy the generated code manually
    add_custom_command(TARGET qhauk_client POST_BUILD
        #COMMAND ${CMAKE_COMMAND} -E rm -rf ${CMAKE_CURRENT_SOURCE_DIR}/api/generated
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_BINARY_DIR}/api/generated
            ${CMAKE_CURRENT_SOURCE_DIR}/api/generated
   )
else()
    message(STATUS "Using pregenerated openapi code")
    file(GLOB client_code
         "api/generated/client/*.cpp"
    )
    target_sources(qhauk_client PRIVATE ${client_code})
    target_include_directories(qhauk_client PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/api/generated
        ${CMAKE_CURRENT_SOURCE_DIR}/api/generated/client
    )
    target_link_libraries(qhauk_client PUBLIC Qt6::Core Qt6::OpenApiCommon)
endif()

add_library(qhauk STATIC
    haukreply.cpp
    qhauk.cpp
    qhaukmockpositionsource.cpp
    qhaukposition.cpp
    qhaukpositionhistory.cpp
    qhauksession.cpp
)
if (IOS)
    set_source_files_properties(qhauksession.cpp PROPERTIES COMPILE_FLAGS "-x objective-c++")
    find_library(UIKIT_LIBRARY UIKit)
    target_link_libraries(qhauk PRIVATE ${UIKIT_LIBRARY})
endif()
target_link_libraries(qhauk PUBLIC qhauk_client utility Qt6::Core Qt6::Positioning Qt6::Quick Qt6::Location)
qt_add_qml_module(qhauk
        URI org.kde.hauk
        VERSION 1.0
)


#ecm_finalize_qml_module(qhauk DESTINATION ${KDE_INSTALL_QMLDIR})

qt_add_executable(qhauk_app MANUAL_FINALIZATION MACOSX_BUNDLE main.cpp)
target_link_libraries(qhauk_app PRIVATE qhaukplugin Qt6::Quick Qt6::QuickControls2)
set_target_properties(qhauk_app
        PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist")

set_source_files_properties(AppSettings.qml PROPERTIES
        QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(qhauk_app
        URI org.kde.hauk.app
        VERSION 1.0
        QML_FILES
        AppSettings.qml
        Main.qml
        Map.qml
        SettingsView.qml
        TouchMapView.qml
)

qt_finalize_executable(qhauk_app)

#ecm_finalize_qml_module(qhauk_app DESTINATION ${KDE_INSTALL_QMLDIR})
