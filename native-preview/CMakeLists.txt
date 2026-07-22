cmake_minimum_required(VERSION 3.20)
project(4d-native-preview VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Vulkan
find_package(Vulkan REQUIRED)
if(NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan SDK required. Install from https://vulkan.lunarg.com/")
endif()

message(STATUS "Vulkan: ${Vulkan_INCLUDE_DIRS}")

# ── Vulkan Producer DLL ────────────────────────────────────────────
# Loaded by the Node.js process via N-API / FFI to export GPU images.

add_library(vulkan_producer SHARED
    src/VulkanProducer.cpp
)

target_include_directories(vulkan_producer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(vulkan_producer PRIVATE
    ${Vulkan_LIBRARIES}
)

set_target_properties(vulkan_producer PROPERTIES
    OUTPUT_NAME "vulkan-producer"
    PREFIX ""
    MSVC_RUNTIME_LIBRARY "MultiThreadedDLL"
)

# ── Preview Executable ──────────────────────────────────────────────
# Standalone Win32 application that imports the producer's GPU images
# and presents them in a resizable Vulkan window.

add_executable(preview_window
    src/PreviewWindow.cpp
)

target_include_directories(preview_window PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(preview_window PRIVATE
    ${Vulkan_LIBRARIES}
    user32
    gdi32
)

set_target_properties(preview_window PROPERTIES
    OUTPUT_NAME "4d-preview"
    WIN32_EXECUTABLE TRUE
    MSVC_RUNTIME_LIBRARY "MultiThreadedDLL"
)

# ── Cross-Process Sync Library (static) ───────────────────────────
# Shared between producer and consumer for handle management,
# device-loss recovery, and daemon restart.

add_library(cross_process_sync STATIC
    src/CrossProcessSync.cpp
)

target_include_directories(cross_process_sync PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(cross_process_sync PUBLIC
    ${Vulkan_LIBRARIES}
)

# ── Test / Verification ────────────────────────────────────────────
# Launches producer + consumer in-process to validate the pipeline.

option(BUILD_TESTS "Build verification tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_executable(test_gpu_preview
        tests/test_gpu_preview.cpp
    )
    target_link_libraries(test_gpu_preview PRIVATE
        vulkan_producer
        cross_process_sync
    )
    add_test(NAME gpu_preview COMMAND test_gpu_preview)
endif()

# ── Install ─────────────────────────────────────────────────────────
install(TARGETS vulkan_producer DESTINATION bin)
install(TARGETS preview_window DESTINATION bin)
install(TARGETS cross_process_sync DESTINATION lib)
install(FILES include/SharedGPUImage.h DESTINATION include)
