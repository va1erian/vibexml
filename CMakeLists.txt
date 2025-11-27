cmake_minimum_required(VERSION 3.16)
project(XmlViewer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(wxBUILD_SHARED "Build wxWidgets as shared library" OFF)
option(wxBUILD_SAMPLES "Build wxWidgets samples" OFF)
option(wxBUILD_TESTS "Build wxWidgets tests" OFF)
option(EMBED_XRC_RESOURCES "Embed XRC resources into executable" ON)

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
    src/EditorSettings.cpp
    src/SettingsDialog.cpp
)

set(HEADERS
    src/MainFrame.h
    src/XmlTreeCtrl.h
    src/XmlEditorCtrl.h
    src/SearchDialog.h
    src/RecentFiles.h
    src/EditorSettings.h
    src/SettingsDialog.h
)

# XRC Resource handling
set(XRC_SOURCE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/resources/dialogs.xrc")

if(EMBED_XRC_RESOURCES)
    # Generated C++ file for embedded XRC
    set(XRC_CPP_FILE "${CMAKE_CURRENT_BINARY_DIR}/generated/xrc_resources.cpp")
    
    # Create output directory at configure time
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
    
    # Custom command to generate C++ from XRC using wxrc
    # Use separate arguments to avoid quoting issues on Windows
    add_custom_command(
        OUTPUT "${XRC_CPP_FILE}"
        COMMAND "$<TARGET_FILE:wxrc>" 
            -c
            -o "${XRC_CPP_FILE}"
            -n InitXrcResources
            "${XRC_SOURCE_FILE}"
        DEPENDS "${XRC_SOURCE_FILE}" wxrc
        COMMENT "Compiling XRC resources into C++ code"
        VERBATIM
    )
    
    # Add generated file to sources
    list(APPEND SOURCES "${XRC_CPP_FILE}")
    
    # Define preprocessor macro to indicate embedded resources
    add_compile_definitions(XRC_EMBEDDED=1)
endif()

# Create executable
# On Windows, use WIN32 to link against WinMain instead of main
if(WIN32)
    add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS} app.manifest)
elseif(APPLE)
    # Create macOS app bundle
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${SOURCES} ${HEADERS})
    
    # Set bundle properties
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "XML Viewer"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.vibexml.xmlviewer"
    )
    
    # If not embedding, copy XRC to bundle Resources
    if(NOT EMBED_XRC_RESOURCES)
        set_source_files_properties("${XRC_SOURCE_FILE}" PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources"
        )
        target_sources(${PROJECT_NAME} PRIVATE "${XRC_SOURCE_FILE}")
    endif()
else()
    add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
endif()

# Link wxWidgets libraries (including xrc for XRC resource support)
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

# Link TinyXML-2 and ensure include directories are set
target_link_libraries(${PROJECT_NAME} PRIVATE
    tinyxml2
)

# If not embedding XRC, copy resources to output directory
if(NOT EMBED_XRC_RESOURCES)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/resources"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${XRC_SOURCE_FILE}"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/resources/dialogs.xrc"
        COMMENT "Copying XRC resources to output directory"
    )
endif()

# Platform-specific post-build steps
if(WIN32 AND wxBUILD_SHARED)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Note: If using shared wxWidgets, ensure DLLs are in PATH or executable directory"
    )
endif()
