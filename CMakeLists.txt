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
    src/VirtualXmlTree.cpp
    src/XmlEditorCtrl.cpp
    src/XmlStreamParser.cpp
    src/SearchDialog.cpp
    src/RecentFiles.cpp
    src/EditorSettings.cpp
    src/SettingsDialog.cpp
    src/FileLoader.cpp
)

set(HEADERS
    src/MainFrame.h
    src/VirtualXmlTree.h
    src/XmlEditorCtrl.h
    src/XmlStreamParser.h
    src/SearchDialog.h
    src/RecentFiles.h
    src/EditorSettings.h
    src/SettingsDialog.h
    src/FileLoader.h
)

# XRC Resource files - one per dialog
set(XRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/SearchDialog.xrc
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/SettingsDialog.xrc
)

# Verify all XRC files exist at configure time
foreach(XRC_FILE ${XRC_FILES})
    if(NOT EXISTS "${XRC_FILE}")
        message(FATAL_ERROR "XRC resource file not found: ${XRC_FILE}")
    endif()
endforeach()

# Generated C++ file for embedded XRC resources
set(XRC_CPP_FILE "${CMAKE_CURRENT_BINARY_DIR}/generated/xrc_resources.cpp")

# Create output directory at configure time
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")

# Custom command to compile XRC files into C++ code using wxrc
add_custom_command(
    OUTPUT "${XRC_CPP_FILE}"
    COMMAND "$<TARGET_FILE:wxrc>" 
        -c
        -o "${XRC_CPP_FILE}"
        -n InitXrcResources
        ${XRC_FILES}
    DEPENDS ${XRC_FILES} wxrc
    COMMENT "Compiling XRC resources into C++ code"
    VERBATIM
)

# Add generated file to sources
list(APPEND SOURCES "${XRC_CPP_FILE}")

# Windows resource file for application icon
set(WIN_RESOURCE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/resources/app.rc")

# Create executable
if(WIN32)
    # Include resource file for Windows icon (only if icon.ico exists)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/icon.ico")
        add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS} ${WIN_RESOURCE_FILE} app.manifest)
    else()
        add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS} app.manifest)
        message(STATUS "Note: resources/icon.ico not found - building without application icon")
    endif()
elseif(APPLE)
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${SOURCES} ${HEADERS})
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "XML Viewer"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.vibexml.xmlviewer"
    )
    # Set icon for macOS bundle (if exists)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/icon.icns")
        set_target_properties(${PROJECT_NAME} PROPERTIES
            MACOSX_BUNDLE_ICON_FILE "icon.icns"
        )
        set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/resources/icon.icns" PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources"
        )
        target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/resources/icon.icns")
    endif()
else()
    add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
endif()

# Link wxWidgets libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    wx::core
    wx::stc
    wx::adv
    wx::base
    wx::xrc
    wx::xml
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/generated
)

# Link TinyXML-2
target_link_libraries(${PROJECT_NAME} PRIVATE
    tinyxml2
)

# UPX compression for Windows Release builds
if(WIN32)
    find_program(UPX_EXECUTABLE upx)
    if(UPX_EXECUTABLE)
        message(STATUS "UPX found: ${UPX_EXECUTABLE}")
        # Add post-build step to compress executable with UPX (Release only)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND $<$<CONFIG:Release>:${UPX_EXECUTABLE}>
                    $<$<CONFIG:Release>:--best>
                    $<$<CONFIG:Release>:--lzma>
                    $<$<CONFIG:Release>:$<TARGET_FILE:${PROJECT_NAME}>>
            COMMENT "Compressing executable with UPX..."
            VERBATIM
        )
    else()
        message(STATUS "UPX not found - executable will not be compressed")
        message(STATUS "  Install UPX from: https://github.com/upx/upx/releases")
    endif()
endif()
