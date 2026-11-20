cmake_minimum_required(VERSION 3.16)
project(win9xplus VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets)
include(GNUInstallDirs)

add_library(win9xplus MODULE
    src/plugin.cpp
    src/win9xplusstyle.cpp
    src/win9xplusstyle.h
)
target_link_libraries(win9xplus PRIVATE Qt6::Widgets)

# Install into Qt's own style plugin folder (e.g. /usr/lib64/qt6/plugins/styles
# on Fedora) and give Plasma's style list a friendly name.
install(TARGETS win9xplus LIBRARY DESTINATION ${QT6_INSTALL_PLUGINS}/styles)
install(FILES win9xplus.themerc DESTINATION ${CMAKE_INSTALL_DATADIR}/kstyle/themes)

option(BUILD_TESTING "Build the offscreen menu tests" ON)
if(BUILD_TESTING)
    add_executable(gaptest tests/gaptest.cpp)
    target_link_libraries(gaptest PRIVATE Qt6::Widgets)

    add_executable(submenutest tests/submenutest.cpp)
    target_link_libraries(submenutest PRIVATE Qt6::Widgets)
endif()
