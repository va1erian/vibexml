# Application Icon Setup

## Required Files

### Windows (icon.ico)
- Multi-resolution ICO file containing:
  - 16x16, 32x32, 48x48, 64x64, 128x128, 256x256 pixels
- Tools to create:
  - [IcoConvert](https://icoconvert.com/) - online converter
  - [GIMP](https://www.gimp.org/) - export as ICO
  - ImageMagick: `convert icon.png -define icon:auto-resize=256,128,64,48,32,16 icon.ico`

### macOS (icon.icns)
- ICNS file with multiple resolutions
- Create using:
  - `iconutil` (macOS built-in)
  - [Icon Generator](https://apps.apple.com/app/icon-generator/id870166485)

### Linux (PNG files)
- Place PNG files in standard locations:
  - `/usr/share/icons/hicolor/256x256/apps/xmlviewer.png`
  - `/usr/share/icons/hicolor/128x128/apps/xmlviewer.png`
  - `/usr/share/icons/hicolor/64x64/apps/xmlviewer.png`
  - `/usr/share/icons/hicolor/48x48/apps/xmlviewer.png`
  - `/usr/share/icons/hicolor/32x32/apps/xmlviewer.png`
  - `/usr/share/icons/hicolor/16x16/apps/xmlviewer.png`

## File Locations

Place the generated files in the `resources/` folder:
```
resources/
├── icon.ico      # Windows
├── icon.icns     # macOS
├── icon-256.png  # Linux/general use
├── icon-128.png
├── icon-64.png
├── icon-48.png
├── icon-32.png
└── icon-16.png
```

## Quick ImageMagick Commands

```bash
# From a single high-res PNG (e.g., 512x512 or 1024x1024):

# Create Windows ICO
convert icon.png -define icon:auto-resize=256,128,64,48,32,16 icon.ico

# Create various PNG sizes
convert icon.png -resize 256x256 icon-256.png
convert icon.png -resize 128x128 icon-128.png
convert icon.png -resize 64x64 icon-64.png
convert icon.png -resize 48x48 icon-48.png
convert icon.png -resize 32x32 icon-32.png
convert icon.png -resize 16x16 icon-16.png

# Create macOS iconset (on macOS)
mkdir icon.iconset
sips -z 16 16 icon.png --out icon.iconset/icon_16x16.png
sips -z 32 32 icon.png --out icon.iconset/icon_16x16@2x.png
sips -z 32 32 icon.png --out icon.iconset/icon_32x32.png
sips -z 64 64 icon.png --out icon.iconset/icon_32x32@2x.png
sips -z 128 128 icon.png --out icon.iconset/icon_128x128.png
sips -z 256 256 icon.png --out icon.iconset/icon_128x128@2x.png
sips -z 256 256 icon.png --out icon.iconset/icon_256x256.png
sips -z 512 512 icon.png --out icon.iconset/icon_256x256@2x.png
sips -z 512 512 icon.png --out icon.iconset/icon_512x512.png
sips -z 1024 1024 icon.png --out icon.iconset/icon_512x512@2x.png
iconutil -c icns icon.iconset
```

## After Adding Icons

Rebuild the project:
```bash
cmake --build . --config Release
```

The Windows executable will automatically have the icon embedded.

