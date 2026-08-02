# LapScroll

**Scroll with your mouse movement. Built for laptops.**

LapScroll is a lightweight Windows utility that lets you scroll by simply moving your mouse vertically after pressing a hotkey. It is designed for users with broken mouse wheels, laptops, or anyone who wants a fast and comfortable way to navigate long pages.

## Features

* 🚀 Lightweight background application
* 🖱️ Scroll by moving the mouse vertically
* ⌨️ Customizable global hotkey
* ⚡ Adjustable scroll speed
* 🎯 Adjustable deadzone
* 💾 Automatically saves your settings
* 📌 Runs quietly in the system tray
* 🔄 Optional Windows startup support *(coming soon)*

## How it works

1. Launch **LapScroll**.
2. Press your configured hotkey.
3. Move your mouse up or down to scroll.
4. Press the hotkey again to return to normal mouse movement.

No extra drivers. No complicated setup.

## Installation

### Option 1 — Installer *(Recommended)*

Download the latest installer from the **Releases** section and follow the setup wizard.

### Option 2 — Portable

1. Download the latest portable ZIP.
2. Extract it anywhere.
3. Run `LapScroll.exe`.

## Settings

You can customize:

* Hotkey
* Scroll speed
* Deadzone

Settings are stored locally and automatically loaded the next time you start LapScroll.

## System Requirements

* Windows 10
* Windows 11

## Roadmap

* [ ] Auto-start with Windows
* [ ] Smooth scrolling mode
* [ ] Tray menu improvements
* [ ] Per-application profiles
* [ ] Automatic updates
* [ ] Multiple scrolling modes

## Building from Source

Compile using MinGW g++:

```bash
windres resource.rc -O coff -o resource.o

g++ LapScroll.cpp resource.o -o LapScroll.exe -mwindows -static -luser32 -lshell32 -ladvapi32

g++ LapScroll.Settings.cpp resource.o -o LapScroll.Settings.exe -mwindows -static -luser32 -lshell32 -ladvapi32
```

## Contributing

Bug reports, feature requests, and pull requests are always welcome.

If you encounter an issue, please open an issue describing:

* Your Windows version
* What happened
* Steps to reproduce the problem

## License

Copyright (c) 2026 Spinx. All Rights Reserved.

This software and its source code may not be copied, modified, redistributed, reverse engineered, or used without explicit written permission from the copyright holder.

---

Made with ❤️ by **Spinx**.
