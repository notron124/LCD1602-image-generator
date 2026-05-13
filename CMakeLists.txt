cmake_minimum_required(VERSION 3.16)

project(lcd1602_image_generator VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2)

qt_standard_project_setup(REQUIRES 6.10)

qt_add_executable(applcd1602_image_generator
    main.cpp
    core/backend/LcdBackend.cpp
    core/backend/LcdBackend.h
)

target_include_directories(applcd1602_image_generator
    PRIVATE
        core/backend
)

qt_add_qml_module(applcd1602_image_generator
    URI lcd1602_image_generator
    QML_FILES
        core/qml/Main.qml
        core/qml/LcdCharacter.qml
        core/qml/Lcd1602.qml
    RESOURCES
        fonts/lcd1602-5x8.bdf
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(applcd1602_image_generator PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.applcd1602_image_generator
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(applcd1602_image_generator
    PRIVATE
        Qt6::Quick
        Qt6::QuickControls2
)

install(TARGETS applcd1602_image_generator
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_app_script(
    TARGET applcd1602_image_generator
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
