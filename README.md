# Fontschau

A font viewer and rendering comparison tool for Linux, written in C++ with Qt6,
FreeType and Fontconfig.

Fontschau is a lightweight alternative to GNOME Font Viewer. It lets you browse the fonts installed on your system, preview them at different sizes and styles, and type your own sample text.

![Fontschau screenshot](screenshot.png)

## Features

- Browse all installed font families
- Open TTF, OTF, and font collection files (File → Open Font, or from the command line)
- Live search / filter
- Select available styles (Regular, Bold, Italic, …)
- Adjustable point size (spinbox + slider)
- Bold / Italic / Underline / Strikeout toggles
- Editable multi-line sample text with European Latin accents, Cyrillic and emoji examples
- Automatic multi-size pangram preview
- Basic font information (family, style, pixel size, Qt exact-match status)
- Switch each preview independently between Qt text rendering and direct FreeType rendering
- FreeType: grayscale, monochrome, RGB/BGR and vertical LCD subpixel rendering;
  hinting, LCD filters, driver stem darkening, synthetic emboldening, coverage gamma
- Qt: platform-dependent antialiasing/subpixel preferences and hinting
- Split the preview into two independent comparison panes, each with its own
  font, style, size, backend and rendering controls
- Clean, straightforward Qt Widgets UI

## Usage

```bash
fontschau --help
fontschau --version
fontschau /path/to/font.ttf
fontschau --font /path/to/font.otf
fontschau -f /path/to/font.ttc
fontschau --renderer freetype --split /path/to/font.ttf
fontschau /path/to/first.ttf /path/to/second.otf
fontschau --font /path/to/first.ttf --font /path/to/second.otf
fontschau --renderer freetype --font /path/to/first.ttf /path/to/second.otf
```

You can also use **File → Open Font** (Ctrl+O). Successfully loaded fonts
are added to the font-family list and selected automatically. An invalid or
unreadable file supplied on the command line produces an error and nonzero exit
status. The desktop launcher can open font files via the file manager.

Passing **two font files** automatically opens the split comparison: the first
font goes directly into **Pane A** (left), and the second into **Pane B**
(right). You can supply two positional paths, repeat `--font`/`-f`, or combine
an option with a positional path (option paths come first). Both fonts keep
their individual source paths, including when the font files have the same
family name. `--renderer` chooses the initial backend for both panes. Supplying
more than two font files is an error. A single font still opens one pane unless
`--split` is specified.

### Comparing renderers and settings

Choose **View → Split Preview** (`Ctrl+\`) to create a second pane. Each
pane has its own settings; clicking its preview or any of its controls selects
which pane the left-hand font browser controls, without a separate selection
button. The previews start at the same height even when one pane uses Qt and
the other shows more FreeType settings. The sample text is shared between panes
to make differences easier to spot. Drag the splitter to adjust their widths;
close pane B to return to the single preview. A newly split pane copies pane A's
settings so comparisons start with identical configurations.

Each pane can select **Qt** or **FreeType** under **Rendering → Engine**.
The Qt renderer uses `QFont` and `QPainter::drawText`, with platform-dependent
antialiasing and hinting *preferences* (it cannot force the physical subpixel
order, filtering, stem darkening or gamma).

The FreeType renderer draws glyph bitmaps directly into an RGB image rather
than going through Qt's font engine. Installed fonts are resolved with
Fontconfig; fonts opened from disk are passed directly to FreeType, including
collection face selection. The available controls are:

- **Antialias:** grayscale, mono, horizontal RGB/BGR, vertical RGB/BGR.
- **Hinting:** default, none, light, full.
- **LCD filter:** none, default, light or legacy (only applies to LCD modes).
- **Stem darkening:** FreeType driver property (may have no effect on some
  drivers/fonts; distinct from the synthetic embolden control).
- **Extra embolden:** synthetic outline thickening in pixels.
- **Coverage gamma:** exponent applied to per-pixel coverage before
  compositing; 1.0 is the neutral setting, not a monitor gamma setting.

**Limitations:** The direct FreeType path is for close inspection of glyph
rasterization, not a replacement for a shaped text layout engine. It implements
basic Unicode character lookup and legacy kerning, but not HarfBuzz shaping,
ligatures, bidirectional layout, or per-glyph font fallback. Consequently,
complex scripts can differ markedly from Qt's rendering. LCD effects are best
compared at 1:1 zoom on a known subpixel arrangement, and depend on FreeType
build support; unavailable LCD raster modes fall back to grayscale.

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

Requirements: Qt 6 (Widgets and Svg), FreeType 2, Fontconfig, pkg-config,
CMake ≥ 3.16, and a C++17 compiler.

## License

GPL-3.0-or-later

## Authors

- Ingo Ruhnke <grumbel@gmail.com>
- Co-authored-by: Grok <grok@x.ai>
