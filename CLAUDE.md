# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Run

```bash
# Configure (Qt path is hardcoded in CMakeLists.txt)
cmake -B build

# Build app
cmake --build build

# Run app
./build/photos.app/Contents/MacOS/photos
```

## Tests

```bash
# Build and run all tests
cmake --build build && ctest --test-dir build

# Run a single test executable
./build/test_ImageEditing
./build/test_ImageIO

# AI tests (only built when ONNXRuntime is found)
./build/test_AIEngine
./build/test_OCR
./build/test_SubjectExtraction
./build/test_TextLayoutAnalyzer
```

Test framework: Qt Test (QTest) for image/editing tests, Google Test (GTest) for AI/ONNX tests.

## Architecture

Desktop photo gallery app built with **C++17 + Qt6 Quick/QML**. Image processing via **OpenCV**; AI inference via **ONNXRuntime** (optional dependency — AI features are conditionally compiled).

### Layers

- **QML UI** (`src/ui/`, `src/Main.qml`) — declarative UI, no business logic
- **C++ bridge** (`src/model/PhotoViewModel`) — the single `Q_INVOKABLE`-heavy class that QML calls into; owns the gallery state, file watcher, and dispatches to subsystems
- **Image subsystems** (`src/image/`) — pure C++/OpenCV, no Qt UI dependency
- **AI subsystems** (`src/ai/`) — ONNX session management; only compiled when `ONNXRuntime_FOUND`
- **Utilities** (`src/util/`) — shared helpers (OCR result cache)
- **Models** (`src/model/`) — Qt data models and the C++/QML bridge

### Key files

| File | Role |
|------|------|
| `src/model/PhotoViewModel.h/cpp` | Central QML↔C++ bridge; gallery state, file watching, editing dispatch, cache management |
| `src/model/GalleryModel.h/cpp` | `QAbstractListModel` for the photo grid |
| `src/model/OcrResultModel.h/cpp` | `QAbstractListModel` exposing OCR results to QML |
| `src/image/AsyncImageProvider.h` | Custom QML image provider for async-cached image display |
| `src/image/ImageEditing.h/cpp` | Crop, rotate, brush drawing (non-destructive) |
| `src/image/ImageEditor.h/cpp` | High-level editing coordinator |
| `src/image/ImageIO.h/cpp` | Static image and GIF frame loading |
| `src/ai/AIEngine.h/cpp` | ONNX Runtime session wrapper |
| `src/ai/OCR.h/cpp` | Singleton OCR pipeline (text detection + recognition) |
| `src/ai/CTCCharBoxExtractor.h/cpp` | CTC decoder producing per-character bounding boxes |
| `src/ai/TextLayoutAnalyzer.h/cpp` | Groups char boxes into lines and words |
| `src/ai/SubjectExtraction.h/cpp` | Background removal pipeline |
| `src/util/OcrCache.h/cpp` | SQLite-backed LRU cache for OCR results (keyed by image MD5) |

### QML UI structure

```
src/Main.qml                        — root Window + StackView navigation
src/ui/
  GalleryView.qml                   — photo grid, top bar with gear button
  PhotoViewer.qml                   — full-screen image viewer
  PhotoEditor.qml                   — editing toolbar (rotate, crop, brush)
  ViewerToolbar.qml                 — viewer top bar (back, zoom, rotate, edit)
  SettingsPage.qml                  — cache management settings
  GifTimeline.qml                   — GIF playback controls
  NavigationButtons.qml             — prev/next arrows
  CropActionBar.qml                 — crop confirm/cancel + aspect ratio pills
  LiveTextOverlay.qml               — OCR text selection overlay
  ImageContextMenu.qml              — right-click context menu
  components/
    BackButton.qml                  — unified 34×34 ghost back-arrow button
    GhostIconButton.qml             — 34×34 ghost icon button (normal/active/destructive)
    ZoomPill.qml                    — [−][scale%][+] zoom control
    Divider.qml                     — 1px horizontal separator
    CardContainer.qml               — dark rounded card (#1C1C1E, r=12)
```

### Patterns to follow

- **Async AI tasks**: heavy work runs in detached threads; results posted back to the UI thread via `QMetaObject::invokeMethod(..., Qt::QueuedConnection)`
- **Thread safety**: OCR pipeline uses a mutex; use `QPointer` when capturing `this` across thread boundaries
- **File watching**: `QFileSystemWatcher` + debounce `QTimer` in `PhotoViewModel` — don't add direct watcher connections elsewhere
- **Image caching**: processed images go through `AsyncImageProvider`; QML references them via custom URL scheme
- **Conditional AI build**: wrap any code touching `AIEngineLib` in `#ifdef` / CMake `if(ONNXRuntime_FOUND)` guards
- **OCR cache**: `OcrCache::instance()` is a thread-safe singleton; access via `PhotoViewModel` invokables from QML
- **QML components**: reuse shared components from `src/ui/components/` — `BackButton`, `GhostIconButton`, `ZoomPill`, `Divider`, `CardContainer`
- **Navigation**: `StackView` in `Main.qml` manages all screen transitions; screens signal `backClicked` / `settingsClicked` etc. upward

### App bundle (macOS)

- ONNX models bundled at `Contents/assets/` via `MACOSX_PACKAGE_LOCATION "assets"` in CMakeLists.txt
- OpenCV and ONNXRuntime dylibs copied into `Contents/Frameworks/` via CMake POST_BUILD commands
- App icon: `assets/AppIcon.svg` → `assets/AppIcon.icns` (built by `scripts/build-icon.sh`)

### Release

GitHub Actions workflow (`.github/workflows/release.yml`) triggers on `v*` tags and uploads:
- macOS: `.dmg` via `macdeployqt`
- Windows: `.zip` via `windeployqt`
