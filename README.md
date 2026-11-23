# Fontschau

A simple font viewer for Linux, written in C++ with Qt6.

Fontschau is a lightweight alternative to GNOME Font Viewer. It lets you browse the fonts installed on your system, preview them at different sizes and styles, and type your own sample text.

![Fontschau screenshot](screenshot.png)

## Features

- Browse all installed font families
- Open TTF, OTF, and font collection files (File → Open Font, or from the command line)
- Live search / filter
- Select available styles (Regular, Bold, Italic, …)
- Adjustable point size (spinbox + slider)
- Bold / Italic / Underline / Strikeout toggles
- Editable multi-line sample text
- Automatic multi-size pangram preview
- Basic font information (family, style, weight, exact match, …)
- Qt-supported rendering preferences: antialiasing, hinting (default/none/vertical/full), and allowing/disabling subpixel antialiasing
- Clean, straightforward Qt Widgets UI

## Usage

```bash
fontschau --help
fontschau --version
fontschau /path/to/font.ttf
fontschau --font /path/to/font.otf
fontschau -f /path/to/font.ttc
```

You can also use **File → Open Font** (Ctrl+O). Successfully loaded fonts
are added to the font-family list and selected automatically. An invalid or
unreadable file supplied on the command line produces an error and nonzero exit
status. The desktop launcher can open font files via the file manager.

The rendering controls use public Qt APIs: `QFont::setHintingPreference()` and
`QFont::setStyleStrategy()`. Their behavior depends on the platform and font
backend; in particular, **Allow subpixel AA** does not force subpixel rendering
when it is disabled by the system. Qt does not expose LCD filter tuning,
FreeType stem darkening, or gamma adjustment through its public font API, so
Fontschau does not attempt to modify these.

## Version numbers

`VERSION` is the only version source. Development versions have a `-dev`
suffix, with a git commit count and short hash added at build time (for example,
`x.y.z-dev.43+gabcdef0`, or `…-dirty`). Nix computes the full version for
packaging and passes it to CMake. Standalone CMake builds derive it from git
when available. For release builds, put the plain release number in `VERSION`,
commit it, and tag that commit with `v` plus the same number. After the release,
bump `VERSION` to the next `-dev` version.

## Building

### With Nix (recommended)

```bash
nix build
./result/bin/fontschau
```

Or enter a development shell:

```bash
nix develop
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
./build/fontschau
```

### With CMake + system Qt6

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
./build/fontschau
```

Requirements: Qt 6 (Widgets and Svg), CMake ≥ 3.16, C++17 compiler.

## License

GPL-3.0-or-later

## Authors

- Ingo Ruhnke <grumbel@gmail.com>
- Co-authored-by: Grok <grok@x.ai>
