cmake_minimum_required(VERSION 3.16)
project(AnythingLLMInstaller VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent)

if (COMMAND qt_standard_project_setup)
    qt_standard_project_setup()
endif()

set(INSTALLER_SOURCES
    src/main.cpp
    src/installerwindow.cpp
    src/installerlogic.cpp
)

set(INSTALLER_HEADERS
    src/installerwindow.h
    src/installerlogic.h
)

qt_add_executable(anything-llm-installer
    ${INSTALLER_SOURCES}
    ${INSTALLER_HEADERS}
    resources/installer.qrc
)

if (NOT DEFINED APP_VERSION)
    set(APP_VERSION "1.9.1")
endif()

target_compile_definitions(anything-llm-installer PRIVATE APP_VERSION="${APP_VERSION}")

target_include_directories(anything-llm-installer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(anything-llm-installer PRIVATE Qt6::Widgets Qt6::Concurrent)

install(TARGETS anything-llm-installer
        RUNTIME DESTINATION bin
        BUNDLE DESTINATION .
        LIBRARY DESTINATION lib)
