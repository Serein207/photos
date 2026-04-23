cmake_minimum_required(VERSION 3.16)

# Parse version from git tag (format: v1.0.0)
execute_process(
    COMMAND git describe --tags --abbrev=0 --match "v*"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_TAG
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(GIT_TAG MATCHES "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)")
    set(VERSION_MAJOR ${CMAKE_MATCH_1})
    set(VERSION_MINOR ${CMAKE_MATCH_2})
    set(VERSION_PATCH ${CMAKE_MATCH_3})
else()
    set(VERSION_MAJOR 0)
    set(VERSION_MINOR 1)
    set(VERSION_PATCH 0)
endif()
set(APP_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

project(photos VERSION ${APP_VERSION} LANGUAGES CXX)
message(STATUS "App version: ${APP_VERSION}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set Qt path as requested
set(CMAKE_PREFIX_PATH "/Users/serein/Qt" ${CMAKE_PREFIX_PATH})

find_package(Qt6 REQUIRED COMPONENTS Core Gui Test Quick QuickControls2 Sql)
find_package(OpenCV REQUIRED)

# Optionally ONNXRuntime (if installed via brew, usually just onnxruntime or we use pkg-config)
find_package(ONNXRuntime QUIET)
if (NOT ONNXRuntime_FOUND)
    # Just a placeholder if it's not strictly required to be found on this machine to pass configure
    # But acceptance criteria says: "CMake configures successfully with Qt path. QTest executable builds and runs"
    # Actually wait, maybe find_library?
    find_library(ONNXRUNTIME_LIBS onnxruntime)
    find_path(ONNXRUNTIME_INCLUDE_DIRS onnxruntime_cxx_api.h PATH_SUFFIXES onnxruntime)
endif()

if (ONNXRUNTIME_LIBS AND ONNXRUNTIME_INCLUDE_DIRS)
    if(NOT TARGET ONNXRuntime::ONNXRuntime)
        add_library(ONNXRuntime::ONNXRuntime UNKNOWN IMPORTED)
        set_target_properties(ONNXRuntime::ONNXRuntime PROPERTIES
            IMPORTED_LOCATION "${ONNXRUNTIME_LIBS}"
            INTERFACE_INCLUDE_DIRECTORIES "${ONNXRUNTIME_INCLUDE_DIRS}"
        )
    endif()
    set(ONNXRuntime_FOUND TRUE)
endif()

# We need CTest
include(CTest)

# AIEngine library
if (ONNXRuntime_FOUND)
    add_library(AIEngineLib
        src/ai/AIEngine.cpp
        src/ai/OCR.cpp
        src/ai/SubjectExtraction.cpp
        src/ai/CTCCharBoxExtractor.cpp
        src/ai/TextLayoutAnalyzer.cpp
    )
    target_include_directories(AIEngineLib PUBLIC src)
    if (TARGET onnxruntime::onnxruntime)
        target_link_libraries(AIEngineLib PUBLIC onnxruntime::onnxruntime ${OpenCV_LIBS} Qt6::Core)
    else()
        target_link_libraries(AIEngineLib PUBLIC ONNXRuntime::ONNXRuntime ${OpenCV_LIBS} Qt6::Core)
    endif()
endif()

# ImageIO library
add_library(ImageIOLib
    src/image/ImageIO.cpp
    src/image/ImageEditing.cpp
    src/image/ImageEditor.cpp
    src/model/GalleryModel.cpp
    src/model/OcrResultModel.cpp
    src/model/OcrResultModel.h
    src/model/PhotoViewModel.cpp
    src/util/OcrCache.cpp
)
set_target_properties(ImageIOLib PROPERTIES AUTOMOC ON)
target_include_directories(ImageIOLib PUBLIC src)
target_link_libraries(ImageIOLib PUBLIC Qt6::Core Qt6::Gui Qt6::Quick Qt6::Qml Qt6::Sql ${OpenCV_LIBS})

if (ONNXRuntime_FOUND)
    target_link_libraries(ImageIOLib PUBLIC AIEngineLib)
endif()

# App executable
qt_standard_project_setup(REQUIRES 6.10)
if(WIN32)
    configure_file(
        ${CMAKE_SOURCE_DIR}/.cmake/AppIcon.rc.in
        ${CMAKE_SOURCE_DIR}/assets/AppIcon.rc
    )
    qt_add_executable(photos src/main.cpp assets/AppIcon.rc assets/app.manifest)

    # Copy ONNX models and dict next to the executable after build
    set(AI_ASSETS
        assets/PP-OCRv5_server_det_infer.onnx
        assets/PP-OCRv5_server_rec_infer.onnx
        assets/ppocrv5_dict.txt
    )
    add_custom_command(TARGET photos POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:photos>/assets"
        VERBATIM
    )
    foreach(_asset ${AI_ASSETS})
        add_custom_command(TARGET photos POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/${_asset}"
                "$<TARGET_FILE_DIR:photos>/assets/"
            VERBATIM
        )
    endforeach()
else()
    qt_add_executable(photos src/main.cpp assets/AppIcon.icns)
    set_target_properties(photos PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "Photos"
        MACOSX_BUNDLE_ICON_FILE AppIcon
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.serein207.photos"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${APP_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${APP_VERSION}"
        MACOSX_BUNDLE_COPYRIGHT "Copyright (C) 2026 Serein207. All rights reserved."
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/.cmake/Info.plist.in"
    )
    set_source_files_properties(assets/AppIcon.icns PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
    # Copy ONNX models and dict into the bundle at Contents/assets/
    set(AI_ASSETS
        assets/PP-OCRv5_server_det_infer.onnx
        assets/PP-OCRv5_server_rec_infer.onnx
        assets/ppocrv5_dict.txt
    )
    target_sources(photos PRIVATE ${AI_ASSETS})
    set_source_files_properties(${AI_ASSETS} PROPERTIES
        MACOSX_PACKAGE_LOCATION "assets"
    )
    # Embed rpaths so the app finds Qt/OpenCV/ONNXRuntime dylibs at runtime
    set_target_properties(photos PROPERTIES
        INSTALL_RPATH "@executable_path/../Frameworks"
        BUILD_WITH_INSTALL_RPATH TRUE
    )

    # Copy OpenCV dylibs into the bundle after build
    foreach(_lib ${OpenCV_LIBS})
        get_target_property(_loc ${_lib} LOCATION)
        if(_loc AND _loc MATCHES "\\.dylib$")
            add_custom_command(TARGET photos POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                    "$<TARGET_BUNDLE_DIR:photos>/Contents/Frameworks"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_loc}"
                    "$<TARGET_BUNDLE_DIR:photos>/Contents/Frameworks/"
                VERBATIM
            )
        endif()
    endforeach()

    # Copy ONNXRuntime dylib into the bundle after build
    if(ONNXRuntime_FOUND AND TARGET ONNXRuntime::ONNXRuntime)
        get_target_property(_onnx_loc ONNXRuntime::ONNXRuntime IMPORTED_LOCATION)
        if(_onnx_loc)
            add_custom_command(TARGET photos POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                    "$<TARGET_BUNDLE_DIR:photos>/Contents/Frameworks"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_onnx_loc}"
                    "$<TARGET_BUNDLE_DIR:photos>/Contents/Frameworks/"
                VERBATIM
            )
        endif()
    endif()
endif()
qt_add_qml_module(photos URI photos QML_FILES
    src/Main.qml
    src/ui/GalleryView.qml
    src/ui/PhotoViewer.qml
    src/ui/PhotoEditor.qml
    src/ui/GifTimeline.qml
    src/ui/ViewerToolbar.qml
    src/ui/NavigationButtons.qml
    src/ui/ImageContextMenu.qml
    src/ui/CropActionBar.qml
    src/ui/LiveTextOverlay.qml
    src/ui/SettingsPage.qml
    src/ui/components/GhostIconButton.qml
    src/ui/components/BackButton.qml
    src/ui/components/ZoomPill.qml
    src/ui/components/Divider.qml
    src/ui/components/CardContainer.qml
)
target_link_libraries(photos PRIVATE Qt6::Quick Qt6::QuickControls2 ImageIOLib)

# Windows: copy OpenCV and ONNXRuntime DLLs next to the executable after build
if(WIN32)
    foreach(_lib ${OpenCV_LIBS})
        get_target_property(_loc ${_lib} IMPORTED_LOCATION_RELEASE)
        if(NOT _loc)
            get_target_property(_loc ${_lib} IMPORTED_LOCATION)
        endif()
        if(_loc AND _loc MATCHES "\\.dll$")
            add_custom_command(TARGET photos POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_loc}"
                    "$<TARGET_FILE_DIR:photos>/"
                VERBATIM
            )
        endif()
    endforeach()

    if(ONNXRuntime_FOUND AND TARGET ONNXRuntime::ONNXRuntime)
        get_target_property(_onnx_loc ONNXRuntime::ONNXRuntime IMPORTED_LOCATION)
        if(_onnx_loc AND _onnx_loc MATCHES "\\.dll$")
            add_custom_command(TARGET photos POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_onnx_loc}"
                    "$<TARGET_FILE_DIR:photos>/"
                VERBATIM
            )
        endif()
    endif()
endif()

# Tests
if(BUILD_TESTING)
    add_executable(test_ImageEditing tests/test_ImageEditing.cpp)
    target_link_libraries(test_ImageEditing PRIVATE ImageIOLib Qt6::Test Qt6::Core Qt6::Gui ${OpenCV_LIBS})
    add_test(NAME test_ImageEditing COMMAND test_ImageEditing)
    add_executable(test_ImageIO tests/test_ImageIO.cpp)
    target_link_libraries(test_ImageIO PRIVATE ImageIOLib Qt6::Test Qt6::Core Qt6::Gui)
    add_test(NAME test_ImageIO COMMAND test_ImageIO)

    if (ONNXRuntime_FOUND)
        # Assuming GTest is available for testing
        find_package(GTest REQUIRED)
        add_executable(test_AIEngine tests/test_AIEngine.cpp)
        target_link_libraries(test_AIEngine PRIVATE AIEngineLib GTest::gtest GTest::gtest_main)
        add_test(NAME test_AIEngine COMMAND test_AIEngine)

        add_executable(test_OCR tests/test_OCR.cpp)
        target_link_libraries(test_OCR PRIVATE AIEngineLib GTest::gtest GTest::gtest_main Qt6::Core)
        add_test(NAME test_OCR COMMAND test_OCR)

        add_executable(test_SubjectExtraction tests/test_SubjectExtraction.cpp)
        target_link_libraries(test_SubjectExtraction PRIVATE AIEngineLib Qt6::Test Qt6::Core Qt6::Gui)
        add_test(NAME test_SubjectExtraction COMMAND test_SubjectExtraction)

        add_executable(test_TextLayoutAnalyzer tests/test_TextLayoutAnalyzer.cpp)
        target_link_libraries(test_TextLayoutAnalyzer PRIVATE AIEngineLib GTest::gtest GTest::gtest_main Qt6::Core)
        add_test(NAME test_TextLayoutAnalyzer COMMAND test_TextLayoutAnalyzer)
    endif()
endif()
