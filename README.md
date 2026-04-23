
# Photos

Desktop photo gallery app built with C++17 + Qt6 Quick/QML, featuring OpenCV image processing and ONNXRuntime AI inference.

## Features

- Modern photo gallery & management
- Non-destructive editing (crop, rotate, brush)
- OCR text recognition & smart selection
- AI background removal (optional)
- Cross-platform (macOS/Windows)

## Build & Run

```bash
cmake -B build
cmake --build build
./build/photos.app/Contents/MacOS/photos
```

## Tests

```bash
cmake --build build && ctest --test-dir build
# Run a single test
./build/test_ImageEditing
./build/test_ImageIO
# AI tests (if ONNXRuntime found)
./build/test_AIEngine
./build/test_OCR
./build/test_SubjectExtraction
./build/test_TextLayoutAnalyzer
```

## Architecture

- QML UI (`src/ui/`, `src/Main.qml`)
- C++/QML bridge (`src/model/PhotoViewModel`)
- Image subsystems (`src/image/`)
- AI subsystems (`src/ai/`)
- Utilities & models (`src/util/`, `src/model/`)

## License

[MIT License](./LICENSE)

