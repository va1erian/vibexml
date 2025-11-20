cmake_minimum_required(VERSION 3.16)
project(XmlViewer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(wxBUILD_SHARED "Build wxWidgets as shared library" OFF)
option(wxBUILD_SAMPLES "Build wxWidgets samples" OFF)
option(wxBUILD_TESTS "Build wxWidgets tests" OFF)

# Fetch wxWidgets using FetchContent
include(FetchContent)

FetchContent_Declare(
    wxWidgets
    GIT_REPOSITORY https://github.com/wxWidgets/wxWidgets.git
    GIT_TAG v3.3.1
    GIT_SHALLOW TRUE
)

# Configure wxWidgets before fetching
set(wxBUILD_SHARED OFF CACHE BOOL "" FORCE)
set(wxBUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(wxBUILD_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(wxWidgets)

# Fetch TinyXML-2
FetchContent_Declare(
    tinyxml2
    GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
    GIT_TAG 9.0.0
    GIT_SHALLOW TRUE
)

# Configure TinyXML-2
FetchContent_MakeAvailable(tinyxml2)

# Platform-specific settings
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS deployment version")
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures for macOS")
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/MainFrame.cpp
    src/XmlTreeCtrl.cpp
    src/XmlEditorCtrl.cpp
    src/SearchDialog.cpp
    src/RecentFiles.cpp
)

set(HEADERS
    src/MainFrame.h
    src/XmlTreeCtrl.h
    src/XmlEditorCtrl.h
    src/SearchDialog.h
    src/RecentFiles.h
)

# Create executable
# On Windows, use WIN32 to link against WinMain instead of main
if(WIN32)
    add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS} app.manifest)
else()
    add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
endif()

# Link wxWidgets libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    wx::core
    wx::stc
    wx::adv
    wx::base
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link TinyXML-2 and ensure include directories are set
# TinyXML-2 target name when fetched via FetchContent
target_link_libraries(${PROJECT_NAME} PRIVATE
    tinyxml2
)

# Platform-specific post-build steps
if(WIN32 AND wxBUILD_SHARED)
    # Copy wxWidgets DLLs to output directory on Windows (only if shared)
    # Note: This is a simplified version - in practice, you may need to adjust
    # based on how wxWidgets is built with FetchContent
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Note: If using shared wxWidgets, ensure DLLs are in PATH or executable directory"
    )
endif()

