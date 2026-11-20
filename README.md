# Win9x Plus

A Qt 6 application style for KDE Plasma 6 that looks like Windows 95/98. It's Qt's built-in
**Windows** style (shown as "MS Windows 9X" in System Settings) with fixes for problems it has
on Plasma 6. The aim is to look as close to real Windows 9x as possible.

Apps look exactly as they do with the built-in style, except for these fixes:

| Problem with the built-in style | Win9x Plus |
| --- | --- |
| The desktop's right-click menu (and other see-through Plasma menus) has a 1px gap between the 3D border and the gray background, so you can see through it. | The gap is filled with the menu's gray. |
| Submenus (e.g. Dolphin's right-click → **Create New**) open on top of the parent menu's 3D border. | Submenus open right against the parent menu's outer edge, on both Wayland and X11. |
| On Wayland, submenus also open a few pixels too low, so their first item doesn't line up with the item you hovered. | The first item lines up with the hovered item, as in Windows 9x. |

## Requirements

- KDE Plasma 6, or any Qt 6 desktop (Qt 6.4 or newer)
- To build: CMake, a C++17 compiler, and Qt 6 development files

Install the build tools:

```bash
# Fedora
sudo dnf install cmake gcc-c++ qt6-qtbase-devel

# Debian / Ubuntu
sudo apt install cmake g++ qt6-base-dev

# Arch
sudo pacman -S cmake gcc qt6-base
```

## Build and install

```bash
git clone https://github.com/TurboOlmo/win9x-plus.git
cd win9x-plus
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
sudo cmake --install build
```

This installs the style into your system's Qt style plugin folder, plus a small file that gives
it the name "Win9x Plus" in System Settings.

## Turn it on

1. Open **System Settings → Colors & Themes → Application Style**.
2. Select **Win9x Plus** and click **Apply**.
3. To make the desktop and panel menus use it without logging out, restart Plasma's shell:
   ```bash
   systemctl --user restart plasma-plasmashell
   ```
   Reopen any other apps you want to switch over.

For the full 9x look, pair it with a 9x-style color scheme, window decoration, and a
non-antialiased font. Those are set separately in System Settings.

## Tests

The `tests/` folder has two small offscreen checks: `gaptest` (no see-through pixels in a
translucent menu) and `submenutest` (submenus don't overlap their parent). Run them from the
build folder:

```bash
mkdir -p build/plugins/styles && cp build/libwin9xplus.so build/plugins/styles/
cd build
QT_QPA_PLATFORM=offscreen QT_PLUGIN_PATH=$PWD/plugins ./gaptest win9xplus
QT_QPA_PLATFORM=offscreen QT_PLUGIN_PATH=$PWD/plugins ./submenutest win9xplus
```

They don't cover Wayland, so submenu placement on Wayland has to be checked on a real desktop.

## AI usage

This project was made with the help of AI. The code, tests, and documentation were written by
Claude (Anthropic's AI model), working in Claude Code, at the direction of the maintainer. The
maintainer found the problems, decided what to fix and how it should look, and tested every fix
on a real Plasma 6 desktop. Commits that Claude helped write are marked with a
`Co-Authored-By: Claude` line.

## License

Win9x Plus uses the same open-source licenses as the Qt style it builds on. You can use it under
**LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only**, whichever you choose. The license texts are in
[`LICENSES/`](LICENSES/).
