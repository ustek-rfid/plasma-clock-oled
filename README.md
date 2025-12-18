# Plasma Clock OLED

A minimal digital clock widget for KDE Plasma on Wayland that repositions randomly to prevent OLED burn-in.

![Plasma Clock OLED](resources/plasma-clock-oled.svg)

## Features

- **OLED Protection** - Randomly repositions every 30 seconds to prevent burn-in
- **KDE Integration** - Reads settings from KDE's Digital Clock applet (time format, date format, seconds display)
- **Panel Aware** - Automatically adapts to panel size, position, and orientation (top, bottom, left, right)
- **Live Updates** - Detects panel configuration changes in real-time
- **Minimal UI** - Transparent background, stays below all windows
- **System Tray** - Optional tray icon with theme-adaptive colors
- **Lightweight** - Native Qt6/C++ application using Wayland layer-shell

## Requirements

- KDE Plasma 6 on Wayland
- Qt 6
- LayerShellQt

## Installation

### Arch Linux

Build and install the package:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target package_arch
sudo pacman -U build/plasma-clock-oled-1.0-1-x86_64.pkg.tar.zst
```

### From Source

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
sudo cmake --install build
```

### Dependencies

- `qt6-base`
- `qt6-wayland`
- `qt6-svg`
- `layer-shell-qt`

## Usage

The application starts automatically on login (via autostart desktop entry).

To start manually:

```bash
plasma-clock-oled
```

### Context Menu

Right-click on the clock or tray icon to access:

- **Hide/Show Tray Icon** - Toggle system tray visibility
- **Quit** - Exit the application

Settings are persisted in `~/.config/Ustek/plasma-clock-oled.conf`

## Configuration

The clock automatically reads settings from KDE's Digital Clock applet:

- Time format (12h/24h/region default)
- Seconds display
- Date visibility and format

To change these settings, configure the Digital Clock widget in your Plasma panel.

## How It Works

The clock widget uses Wayland's layer-shell protocol to position itself on the bottom layer, below all windows including the panel. Every 30 seconds, it moves to a random horizontal position (or vertical for side panels) within the panel bounds, preventing static elements from causing OLED burn-in.

## License

MIT License - see [LICENSE](LICENSE)

## Author

[Ustek RFID](https://github.com/ustek-rfid)
