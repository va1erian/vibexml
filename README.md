# XML Viewer

A cross-platform desktop application for conveniently reading large XML files, built with wxWidgets.

## Features

- **Syntax Highlighting**: XML syntax highlighting with color-coded tags, attributes, comments, and text
- **Tree Navigation**: Left panel tree view showing the XML document structure
- **Attribute Display**: View element attributes when hovering or selecting tree nodes
- **Text Content**: View text content of elements via context menu
- **Search Functionality**: Find text with case-sensitive and whole-word matching options
- **Recent Files**: Automatically tracks and provides quick access to recently opened files
- **Large File Support**: Optimized for handling large XML files efficiently

## Prerequisites

### macOS

1. **Install CMake** (3.16 or later):
   ```bash
   brew install cmake
   ```

2. **Install Xcode Command Line Tools**:
   ```bash
   xcode-select --install
   ```

### Windows

1. **Install CMake** (3.16 or later):
   - Download from [cmake.org](https://cmake.org/download/)
   - Or use Chocolatey: `choco install cmake`

2. **Install a C++ Compiler**:
   - **Option A**: Visual Studio 2019 or later (with C++ desktop development workload)
   - **Option B**: MinGW-w64
     - Download from [mingw-w64.org](https://www.mingw-w64.org/)
     - Or use MSYS2: `pacman -S mingw-w64-x86_64-gcc`

## Building

The project uses CMake with FetchContent to automatically download and build wxWidgets, so you don't need to install wxWidgets separately.

### macOS

1. **Clone or navigate to the project directory**:
   ```bash
   cd vibexml
   ```

2. **Create a build directory**:
   ```bash
   mkdir build
   cd build
   ```

3. **Configure with CMake**:
   ```bash
   cmake ..
   ```

4. **Build the project**:
   ```bash
   cmake --build . --config Release
   ```

5. **Run the application**:
   ```bash
   ./XmlViewer
   ```

   Or open the generated Xcode project:
   ```bash
   open XmlViewer.xcodeproj
   ```

### Windows

#### Using Visual Studio

1. **Open Command Prompt or PowerShell** and navigate to the project directory:
   ```cmd
   cd vibexml
   ```

2. **Create a build directory**:
   ```cmd
   mkdir build
   cd build
   ```

3. **Configure with CMake** (generates Visual Studio solution):
   ```cmd
   cmake .. -G "Visual Studio 17 2022" -A x64
   ```
   (Adjust the generator name for your Visual Studio version)

4. **Build the project**:
   ```cmd
   cmake --build . --config Release
   ```

5. **Run the application**:
   ```cmd
   Release\XmlViewer.exe
   ```

   Or open the generated solution in Visual Studio:
   ```cmd
   start XmlViewer.sln
   ```

#### Using MinGW

1. **Open MSYS2 MinGW 64-bit terminal** and navigate to the project directory:
   ```bash
   cd /c/path/to/vibexml
   ```

2. **Create a build directory**:
   ```bash
   mkdir build
   cd build
   ```

3. **Configure with CMake**:
   ```bash
   cmake .. -G "MinGW Makefiles"
   ```

4. **Build the project**:
   ```bash
   cmake --build . --config Release
   ```

5. **Run the application**:
   ```bash
   ./XmlViewer.exe
   ```

## Usage

### Opening Files

- **File → Open** (Ctrl+O): Open a new XML file
- **File → Recent Files**: Access recently opened files

### Navigation

- **Tree Panel**: Click on any node in the left tree panel to jump to that location in the XML document
- **Context Menu**: Right-click on a tree node to view text content

### Search

- **Edit → Find** (Ctrl+F): Open search dialog
- **Edit → Find Next** (F3): Find next occurrence
- **Search Options**:
  - Case sensitive: Match exact case
  - Whole words only: Match complete words only

### View

- **View → Tree Panel** (Ctrl+T): Toggle visibility of the tree panel

## Project Structure

```
vibexml/
├── CMakeLists.txt          # Main CMake configuration
├── README.md               # This file
└── src/
    ├── main.cpp            # Application entry point
    ├── MainFrame.h/cpp     # Main application window
    ├── XmlTreeCtrl.h/cpp   # XML tree view control
    ├── XmlEditorCtrl.h/cpp # XML text editor with syntax highlighting
    ├── SearchDialog.h/cpp  # Search dialog
    └── RecentFiles.h/cpp   # Recent files management
```

## Configuration

Recent files are stored in:
- **macOS**: `~/Library/Preferences/XmlViewer/recentfiles.ini`
- **Windows**: `%APPDATA%\XmlViewer\recentfiles.ini`

## Troubleshooting

### Build Issues

- **wxWidgets not found**: The project uses FetchContent to download wxWidgets automatically. Ensure you have an internet connection during the first build.
- **CMake version**: Ensure you have CMake 3.16 or later.
- **Compiler issues**: Make sure your C++ compiler supports C++17.

### Runtime Issues

- **Application won't start**: On Windows, ensure all required DLLs are in the same directory as the executable (CMake should copy them automatically).
- **Large files are slow**: The application is optimized for large files, but extremely large files (>100MB) may take time to load and parse.

## License

This project is provided as-is for educational and personal use.

