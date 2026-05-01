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
| `src/ai/SubjectExtraction.h/cpp` | Background removal pipeline (MobileSAM + GrabCut fallback) |
| `src/ai/SAMPipeline.h/cpp` | MobileSAM encoder+decoder ONNX inference |
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
- **Thread safety**: both OCR and SAMPipeline use `std::mutex` to protect ONNX sessions from concurrent `Run()` calls. Use `QPointer` when capturing `this` across thread boundaries.
- **File watching**: `QFileSystemWatcher` + debounce `QTimer` in `PhotoViewModel` — don't add direct watcher connections elsewhere
- **Image caching**: processed images go through `AsyncImageProvider`; QML references them via custom URL scheme
- **Conditional AI build**: wrap any code touching `AIEngineLib` in `#ifdef` / CMake `if(ONNXRuntime_FOUND)` guards
- **OCR cache**: `OcrCache::instance()` is a thread-safe singleton; access via `PhotoViewModel` invokables from QML
- **QML components**: reuse shared components from `src/ui/components/` — `BackButton`, `GhostIconButton`, `ZoomPill`, `Divider`, `CardContainer`
- **Navigation**: `StackView` in `Main.qml` manages all screen transitions; screens signal `backClicked` / `settingsClicked` etc. upward

### ONNX model conventions

- Models stored in `assets/`, committed with Git LFS. Path at runtime: `QCoreApplication::applicationDirPath() + "../assets/<model>.onnx"`
- MobileSAM uses two models (samexporter format): encoder (`mobile_sam_encoder.onnx`, NHWC [H,W,3] no batch dim) + decoder (`mobile_sam_decoder.onnx`)
- Download HuggingFace models via hf-mirror.com when GFW blocks: `curl -L -o output "https://hf-mirror.com/<user>/<repo>/resolve/main/<file>"`
- ONNX Runtime C++ `Run()` requires explicit output names — `(nullptr, 0)` is "0 outputs" not "all outputs"; query with `GetOutputNameAllocated(0, allocator)`
- Encoder input name is queried dynamically (`GetInputNameAllocated`), decoder input names are fixed constants matching the samexporter spec
- **SAMPipeline gotchas**: use single center-point prompt (9-point grid causes fg/bg swap); do NOT normalize encoder input (raw 0-255 float32); do NOT sigmoid decoder output (already probabilities); border fill must be gray `Scalar(128,128,128)`; auto-invert mask when center pixel <0.5; pick best of 3 masks by mean closest to 0.5 after clamp
- **Tilted text highlighting**: LiveTextOverlay computes per-line average rotation angle from CharBox `RotatedRect.angle`, draws highlight rects with `ctx.translate`+`ctx.rotate` in Canvas 2D

### Performance benchmark

- `tests/test_Benchmark.cpp` — QtTest benchmark for image IO, editing, OCR pipeline, and subject extraction timing
- Run: `cmake --build build --target test_Benchmark && ./build/test_Benchmark`

### LaTeX / thesis

- Template: `thesis/njupthesis.cls` adapted for macOS fonts (SimSun→Songti SC, SimHei→Heiti SC)
- Missing TeX Live packages: `tlmgr --usermode install <pkg>`
- Compile: `cd thesis && xelatex main && bibtex main && xelatex main && xelatex main`
- Table format: 2 decimal places, unit in header (`/ ms`), no `n=` prefix

### TikZ diagrams

- No nested `\node` inside `\node` — use independent nodes via `(box.center -| col)` grid alignment
- Max width ~12.5cm for A4 3cm margins; vertical flow preferred for pipelines

### App bundle (macOS)

- ONNX models bundled at `Contents/assets/` via `MACOSX_PACKAGE_LOCATION "assets"` in CMakeLists.txt
- OpenCV and ONNXRuntime dylibs copied into `Contents/Frameworks/` via CMake POST_BUILD commands
- App icon: `assets/AppIcon.svg` → `assets/AppIcon.icns` (built by `scripts/build-icon.sh`)

### Release

GitHub Actions workflow (`.github/workflows/release.yml`) triggers on `v*` tags and uploads:
- macOS: `.dmg` via `macdeployqt`
- Windows: `.zip` via `windeployqt`
