#pragma once

#include <cstdint>
#include <string>
#include <memory>

// Shared GPU Image Protocol v1
// Defines the cross-process contract between the Vulkan producer (renderer)
// and the Vulkan/DXGI consumer (preview window).

// ── Protocol Version ───────────────────────────────────────────────

constexpr uint32_t SHARED_GPU_IMAGE_MAGIC = 0x47505553; // "SXUP" little-endian
constexpr uint32_t SHARED_GPU_IMAGE_VERSION = 1;

// ── Image Format Enum ──────────────────────────────────────────────

enum class SharedImageFormat : uint32_t {
    UNDEFINED = 0,
    R8G8B8A8_UNORM = 1,
    R8G8B8A8_SRGB = 2,
    B8G8R8A8_UNORM = 3,
    B8G8R8A8_SRGB = 4,
    R16G16B16A16_SFLOAT = 5,
    R16G16B16A16_UNORM = 6,
    R32G32B32A32_SFLOAT = 7,
};

// ── Resource Type ──────────────────────────────────────────────────

enum class SharedResourceType : uint32_t {
    INVALID = 0,
    VULKAN_IMAGE = 1,      // VkImage with exportable Win32 handle
    VULKAN_SEMAPHORE = 2,  // VkSemaphore with exportable Win32 handle
    DXGI_SHARED_RESOURCE = 3, // IDXGIResource for D3D11 interop
};

// ── Handle Descriptor ──────────────────────────────────────────────

struct SharedHandleDescriptor {
    SharedResourceType type = SharedResourceType::INVALID;
    uint32_t handleValue = 0;       // The raw HANDLE value (process-local)
    uint64_t sharedHandleHash = 0;  // Hash for cross-process verification
    char namedHandle[64] = {};      // Named kernel object for cross-process access

    std::string namedHandleStr() const { return std::string(namedHandle); }
};

// ── Frame Descriptor (per-frame metadata) ──────────────────────────

struct SharedFrameDescriptor {
    uint32_t frameIndex = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    SharedImageFormat format = SharedImageFormat::R8G8B8A8_UNORM;
    uint64_t presentationTimeNs = 0;
    uint32_t activeSlot = 0;   // Double-buffer slot index
    bool valid = false;
};

// ── Producer Config (set by JS side, read by native producer) ──────

struct ProducerConfig {
    uint32_t width = 1920;
    uint32_t height = 1080;
    SharedImageFormat format = SharedImageFormat::R8G8B8A8_UNORM;
    uint32_t doubleBufferSlots = 2;   // 2 or 3
    bool enableVsync = false;
    const char* previewProcessPath = nullptr;
    const char* instanceName = "4d-renderer";  // Namespace for named handles
    uint32_t timeoutMs = 5000;
    bool debugMode = false;
};

// ── Consumer Config (set by preview process) ────────────────────────

struct ConsumerConfig {
    uint32_t windowWidth = 1280;
    uint32_t windowHeight = 720;
    const char* windowTitle = "4D Renderer Preview";
    bool useDXGIInterop = false;  // false = Vulkan swapchain, true = DXGI
    bool enableVsync = true;
    const char* instanceName = "4d-renderer";
};

// ── Named Handle Conventions ────────────────────────────────────────
// These are the kernel-object names used for cross-process communication.
// InstanceName is user-configurable; defaults to "4d-renderer".

// Format: Global\<InstanceName>_<resource>_<slot>

inline std::string MakeImageHandleName(const char* instance, uint32_t slot) {
    return std::string("Global\\") + instance + "_image_" + std::to_string(slot);
}

inline std::string MakeSemaphoreHandleName(const char* instance, uint32_t slot) {
    return std::string("Global\\") + instance + "_sem_" + std::to_string(slot);
}

inline std::string MakeFenceHandleName(const char* instance) {
    return std::string("Global\\") + instance + "_fence";
}

inline std::string MakeConfigHandleName(const char* instance) {
    return std::string("Global\\") + instance + "_config";
}

// ── Shared Config Block ─────────────────────────────────────────────
// Written by producer, read by consumer. Stored in a named shared
// memory section so the preview process can discover dimensions/format.

struct alignas(64) SharedConfigBlock {
    uint32_t magic = SHARED_GPU_IMAGE_MAGIC;
    uint32_t version = SHARED_GPU_IMAGE_VERSION;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t format = static_cast<uint32_t>(SharedImageFormat::R8G8B8A8_UNORM);
    uint32_t doubleBufferSlots = 2;
    uint32_t activeSlot = 0;
    uint64_t frameCount = 0;
    uint64_t lastFrameTimeNs = 0;
    uint32_t producerPID = 0;
    uint32_t consumerPID = 0;
    uint32_t producerStatus = 0; // 0=idle, 1=rendering, 2=ready, 3=error
    uint32_t consumerStatus = 0; // 0=disconnected, 1=connected, 2=presenting
    uint32_t flags = 0;          // Bitfield for resize, cancel, etc.
    uint64_t reserved[8] = {};
};

// ── Protocol Flags ──────────────────────────────────────────────────

constexpr uint32_t FLAG_RESIZE_PENDING = 1u << 0;
constexpr uint32_t FLAG_CANCEL_PENDING = 1u << 1;
constexpr uint32_t FLAG_DEVICE_LOST = 1u << 2;
constexpr uint32_t FLAG_RESTART_REQUESTED = 1u << 3;
constexpr uint32_t FLAG_CONSUMER_READY = 1u << 4;

// ── Status Codes ────────────────────────────────────────────────────

// Producer
constexpr uint32_t PRODUCER_IDLE = 0;
constexpr uint32_t PRODUCER_RENDERING = 1;
constexpr uint32_t PRODUCER_FRAME_READY = 2;
constexpr uint32_t PRODUCER_ERROR = 3;

// Consumer
constexpr uint32_t CONSUMER_DISCONNECTED = 0;
constexpr uint32_t CONSUMER_CONNECTED = 1;
constexpr uint32_t CONSUMER_PRESENTING = 2;
constexpr uint32_t CONSUMER_ERROR = 3;

// ── Semaphore Timeline Values ───────────────────────────────────────

// Using timeline semaphores for cross-process synchronization:
//   Producer signals: producer_done[slot]
//   Consumer waits:   producer_done[slot]
//   Consumer signals: consumer_ready[slot]
//   Producer waits:   consumer_ready[slot]

constexpr uint64_t SEM_WAIT_INFINITE = UINT64_MAX;

// ── Error Codes ─────────────────────────────────────────────────────

enum class SharedGPUError : uint32_t {
    SUCCESS = 0,
    HANDLE_NOT_FOUND = 1,
    HANDLE_ACCESS_DENIED = 2,
    DEVICE_LOST = 3,
    TIMEOUT = 4,
    INVALID_PARAMETER = 5,
    OUT_OF_MEMORY = 6,
    FORMAT_UNSUPPORTED = 7,
    PROCESS_NOT_FOUND = 8,
    INTERNAL_ERROR = 9,
};
