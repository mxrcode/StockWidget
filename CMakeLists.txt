cmake_minimum_required(VERSION 3.5)

project(StockWidget VERSION 1.3.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Core Gui Network Concurrent)

include(FetchContent)

FetchContent_Declare(
  minizip
  GIT_REPOSITORY https://github.com/zlib-ng/minizip-ng
  GIT_TAG 4.0.0
)

FetchContent_MakeAvailable(minizip)

set(PROJECT_SOURCES
        src/main.cpp
        src/mainwindow.cpp src/mainwindow.h src/mainwindow.ui
        src/configurator.h src/configurator.cpp src/configurator.ui
        src/aboutqt.h src/aboutqt.cpp src/aboutqt.ui
        src/aboutme.h src/aboutme.cpp src/aboutme.ui
        src/warningui.h src/warningui.cpp src/warningui.ui
        src/version.h
        src/qvlabel.hpp
)

qt_add_executable(StockWidget
    MANUAL_FINALIZATION
    resources/application.qrc
    resources/recources.rc
    ${PROJECT_SOURCES}
)

target_link_libraries(StockWidget PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Concurrent minizip)

set_target_properties(StockWidget PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS StockWidget
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(StockWidget)
endif()
