#include "../include/SharedGPUImage.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

#include <windows.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <string>
#include <stdexcept>
#include <sstream>

// ── Helpers ─────────────────────────────────────────────────────────

static void VK_CHECK(VkResult res, const char* msg) {
    if (res != VK_SUCCESS) {
        std::stringstream ss; ss << msg << ": " << res;
        throw std::runtime_error(ss.str());
    }
}

// ── VulkanProducer ──────────────────────────────────────────────────

class VulkanProducer {
public:
    VulkanProducer(const ProducerConfig& cfg)
        : config_(cfg)
        , instance_(VK_NULL_HANDLE)
        , physicalDevice_(VK_NULL_HANDLE)
        , device_(VK_NULL_HANDLE)
        , graphicsQueue_(VK_NULL_HANDLE)
        , graphicsQueueFamily_(UINT32_MAX)
        , configFileMapping_(nullptr)
        , configView_(nullptr)
    {
        imageSlots_.resize(config_.doubleBufferSlots);
        imageMemory_.resize(config_.doubleBufferSlots);
        imageHandles_.resize(config_.doubleBufferSlots);
        semaphoreHandles_.resize(config_.doubleBufferSlots);
    }

    ~VulkanProducer() {
        shutdown();
    }

    void init() {
        createInstance();
        pickPhysicalDevice();
        createDevice();
        getQueues();
        createExportableImages();
        createExportableSemaphores();
        createConfigSharedMemory();
        writeConfigBlock(0, 0);
        std::cout << "[VulkanProducer] Initialized. "
                  << config_.width << "x" << config_.height
                  << " format=" << static_cast<uint32_t>(config_.format)
                  << " slots=" << config_.doubleBufferSlots << std::endl;
    }

    void shutdown() {
        if (device_ != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(device_);
        }
        destroyExportableSemaphores();
        destroyExportableImages();
        destroyConfigSharedMemory();
        if (device_ != VK_NULL_HANDLE) {
            vkDestroyDevice(device_, nullptr);
            device_ = VK_NULL_HANDLE;
        }
        if (instance_ != VK_NULL_HANDLE) {
            vkDestroyInstance(instance_, nullptr);
            instance_ = VK_NULL_HANDLE;
        }
    }

    // ── Per-Frame Export ──────────────────────────────────────────

    uint32_t beginFrame() {
        if (deviceLost_) return UINT32_MAX;
        currentSlot_ = (currentSlot_ + 1) % config_.doubleBufferSlots;
        waitForConsumer(currentSlot_);
        return currentSlot_;
    }

    void endFrame(uint32_t slot) {
        if (deviceLost_) return;
        writeConfigBlock(slot, ++frameCount_);
        signalConsumer(slot);
    }

    // ── Handle Access ─────────────────────────────────────────────

    void* getImageHandle(uint32_t slot) const {
        if (slot >= imageHandles_.size()) return nullptr;
        return imageHandles_[slot];
    }

    void* getSemaphoreHandle(uint32_t slot) const {
        if (slot >= semaphoreHandles_.size()) return nullptr;
        return semaphoreHandles_[slot];
    }

    uint32_t width() const { return config_.width; }
    uint32_t height() const { return config_.height; }
    uint32_t slots() const { return config_.doubleBufferSlots; }
    uint32_t currentSlot() const { return currentSlot_; }
    uint64_t frameCount() const { return frameCount_; }
    bool deviceLost() const { return deviceLost_; }

    // ── Config Block ──────────────────────────────────────────────

    SharedConfigBlock* configBlock() {
        return reinterpret_cast<SharedConfigBlock*>(configView_);
    }

    // ── Resize ────────────────────────────────────────────────────

    void resize(uint32_t width, uint32_t height) {
        vkDeviceWaitIdle(device_);
        destroyExportableImages();
        config_.width = width;
        config_.height = height;
        createExportableImages();
        writeConfigBlock(currentSlot_, frameCount_);
        configBlock()->flags &= ~FLAG_RESIZE_PENDING;
        std::cout << "[VulkanProducer] Resized to " << width << "x" << height << std::endl;
    }

private:
    ProducerConfig config_;
    bool deviceLost_ = false;
    uint32_t currentSlot_ = 0;
    uint64_t frameCount_ = 0;

    // Vulkan objects
    VkInstance instance_;
    VkPhysicalDevice physicalDevice_;
    VkDevice device_;
    VkQueue graphicsQueue_;
    uint32_t graphicsQueueFamily_;

    // Per-slot resources
    std::vector<VkImage> imageSlots_;
    std::vector<VkDeviceMemory> imageMemory_;
    std::vector<HANDLE> imageHandles_;
    std::vector<VkSemaphore> producerSemaphores_;
    std::vector<VkSemaphore> consumerSemaphores_;
    std::vector<HANDLE> semaphoreHandles_;

    // Config shared memory
    HANDLE configFileMapping_ = nullptr;
    void* configView_ = nullptr;

    // ── Instance ──────────────────────────────────────────────────

    void createInstance() {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "4D-VulkanProducer";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        const char* extensions[] = {
            VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
            VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME,
            "VK_KHR_win32_surface",
            "VK_KHR_surface",
#ifdef VK_EXT_DEBUG_UTILS_EXTENSION_NAME
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
        };

        VkInstanceCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        info.pApplicationInfo = &appInfo;
        info.enabledExtensionCount = static_cast<uint32_t>(std::size(extensions));
        info.ppEnabledExtensionNames = extensions;

        VK_CHECK(vkCreateInstance(&info, nullptr, &instance_), "vkCreateInstance");
    }

    void pickPhysicalDevice() {
        uint32_t count = 0;
        VK_CHECK(vkEnumeratePhysicalDevices(instance_, &count, nullptr), "vkEnumeratePhysicalDevices");
        if (count == 0) throw std::runtime_error("No Vulkan physical devices");

        std::vector<VkPhysicalDevice> devices(count);
        VK_CHECK(vkEnumeratePhysicalDevices(instance_, &count, devices.data()), "vkEnumeratePhysicalDevices");

        for (size_t i = 0; i < devices.size(); i++) {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(devices[i], &props);

            uint32_t qCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &qCount, nullptr);
            std::vector<VkQueueFamilyProperties> queues(qCount);
            vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &qCount, queues.data());

            for (uint32_t q = 0; q < qCount; q++) {
                if (queues[q].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    VkPhysicalDeviceExternalMemoryHostPropertiesEXT extMemProps{};
                    // Check for required extensions
                    uint32_t extCount = 0;
                    vkEnumerateDeviceExtensionProperties(devices[i], nullptr, &extCount, nullptr);
                    std::vector<VkExtensionProperties> exts(extCount);
                    vkEnumerateDeviceExtensionProperties(devices[i], nullptr, &extCount, exts.data());

                    bool hasExternalMem = false, hasExternalSem = false;
                    for (const auto& e : exts) {
                        if (strcmp(e.extensionName, VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME) == 0) hasExternalMem = true;
                        if (strcmp(e.extensionName, VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME) == 0) hasExternalSem = true;
                    }

                    if (hasExternalMem && hasExternalSem) {
                        physicalDevice_ = devices[i];
                        graphicsQueueFamily_ = q;
                        VkPhysicalDeviceProperties props;
                        vkGetPhysicalDeviceProperties(physicalDevice_, &props);
                        std::cout << "[VulkanProducer] Selected: "
                                  << props.deviceName
                                  << " (queue family " << q << ")" << std::endl;
                        return;
                    }
                }
            }
        }
        throw std::runtime_error("No suitable Vulkan device found (needs VK_KHR_external_memory_win32 + _semaphore_win32)");
    }

    void createDevice() {
        const float queuePriority = 1.0f;
        VkDeviceQueueCreateInfo queueInfo{};
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueFamilyIndex = graphicsQueueFamily_;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &queuePriority;

        const char* extensions[] = {
            VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME,
            VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
            VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME,
            VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME,
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };

        VkPhysicalDeviceFeatures features{};
        features.samplerAnisotropy = VK_TRUE;

        VkDeviceCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        info.queueCreateInfoCount = 1;
        info.pQueueCreateInfos = &queueInfo;
        info.enabledExtensionCount = static_cast<uint32_t>(std::size(extensions));
        info.ppEnabledExtensionNames = extensions;
        info.pEnabledFeatures = &features;

        VK_CHECK(vkCreateDevice(physicalDevice_, &info, nullptr, &device_), "vkCreateDevice");
    }

    void getQueues() {
        vkGetDeviceQueue(device_, graphicsQueueFamily_, 0, &graphicsQueue_);
    }

    // ── Exportable Images ─────────────────────────────────────────

    void createExportableImages() {
        for (uint32_t i = 0; i < config_.doubleBufferSlots; i++) {
            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.format = toVkFormat(config_.format);
            imageInfo.extent = { config_.width, config_.height, 1 };
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            VkExternalMemoryImageCreateInfo extInfo{};
            extInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
            extInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;

            imageInfo.pNext = &extInfo;

            VK_CHECK(vkCreateImage(device_, &imageInfo, nullptr, &imageSlots_[i]), "vkCreateImage");

            VkMemoryRequirements memReqs;
            vkGetImageMemoryRequirements(device_, imageSlots_[i], &memReqs);

            VkExportMemoryAllocateInfo exportInfo{};
            exportInfo.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
            exportInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.pNext = &exportInfo;
            allocInfo.allocationSize = memReqs.size;
            allocInfo.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            VK_CHECK(vkAllocateMemory(device_, &allocInfo, nullptr, &imageMemory_[i]), "vkAllocateMemory");
            VK_CHECK(vkBindImageMemory(device_, imageSlots_[i], imageMemory_[i], 0), "vkBindImageMemory");

            // Export the Win32 handle
            VkMemoryGetWin32HandleInfoKHR getHandleInfo{};
            getHandleInfo.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
            getHandleInfo.memory = imageMemory_[i];
            getHandleInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;

            auto vkGetMemoryWin32HandleKHR = reinterpret_cast<PFN_vkGetMemoryWin32HandleKHR>(
                vkGetDeviceProcAddr(device_, "vkGetMemoryWin32HandleKHR"));
            if (!vkGetMemoryWin32HandleKHR) throw std::runtime_error("vkGetMemoryWin32HandleKHR not found");

            HANDLE h;
            VK_CHECK(vkGetMemoryWin32HandleKHR(device_, &getHandleInfo, &h), "vkGetMemoryWin32HandleKHR");

            // Duplicate into a named handle for the consumer
            std::string name = MakeImageHandleName(config_.instanceName, i);
            HANDLE namedHandle = CreateFileMapping(
                INVALID_HANDLE_VALUE,
                nullptr,
                PAGE_READWRITE,
                0,
                static_cast<DWORD>(memReqs.size),
                name.c_str()
            );

            if (!namedHandle) {
                // Already exists from a previous instance — open it
                namedHandle = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, name.c_str());
            }

            // Map the Vulkan memory into the file mapping
            // (In production, use DuplicateHandle or a shared memory section)
            imageHandles_[i] = h;

            std::cout << "[VulkanProducer] Slot " << i
                      << " image created, handle=" << h
                      << " named=" << name << std::endl;
        }
    }

    void destroyExportableImages() {
        for (uint32_t i = 0; i < imageSlots_.size(); i++) {
            if (imageSlots_[i] != VK_NULL_HANDLE) {
                vkDestroyImage(device_, imageSlots_[i], nullptr);
                imageSlots_[i] = VK_NULL_HANDLE;
            }
            if (imageMemory_[i] != VK_NULL_HANDLE) {
                vkFreeMemory(device_, imageMemory_[i], nullptr);
                imageMemory_[i] = VK_NULL_HANDLE;
            }
            if (imageHandles_[i]) {
                CloseHandle(imageHandles_[i]);
                imageHandles_[i] = nullptr;
            }
        }
    }

    // ── Exportable Semaphores ─────────────────────────────────────

    void createExportableSemaphores() {
        for (uint32_t i = 0; i < config_.doubleBufferSlots; i++) {
            VkSemaphoreTypeCreateInfo timelineInfo{};
            timelineInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
            timelineInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
            timelineInfo.initialValue = 0;

            VkExportSemaphoreCreateInfo exportInfo{};
            exportInfo.sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO;
            exportInfo.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;

            VkSemaphoreCreateInfo semInfo{};
            semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            semInfo.pNext = &exportInfo;
            semInfo.flags = 0;

            VK_CHECK(vkCreateSemaphore(device_, &semInfo, nullptr, &consumerSemaphores_.emplace_back()), "vkCreateSemaphore");
            VK_CHECK(vkCreateSemaphore(device_, &semInfo, nullptr, &producerSemaphores_.emplace_back()), "vkCreateSemaphore");

            // Export
            auto vkGetSemaphoreWin32HandleKHR = reinterpret_cast<PFN_vkGetSemaphoreWin32HandleKHR>(
                vkGetDeviceProcAddr(device_, "vkGetSemaphoreWin32HandleKHR"));
            if (!vkGetSemaphoreWin32HandleKHR) throw std::runtime_error("vkGetSemaphoreWin32HandleKHR not found");

            VkSemaphoreGetWin32HandleInfoKHR getInfo{};
            getInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR;
            getInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;

            getInfo.semaphore = consumerSemaphores_[i];
            HANDLE hConsumer;
            VK_CHECK(vkGetSemaphoreWin32HandleKHR(device_, &getInfo, &hConsumer), "vkGetSemaphoreWin32HandleKHR");

            getInfo.semaphore = producerSemaphores_[i];
            HANDLE hProducer;
            VK_CHECK(vkGetSemaphoreWin32HandleKHR(device_, &getInfo, &hProducer), "vkGetSemaphoreWin32HandleKHR");

            semaphoreHandles_[i] = hConsumer;

            std::cout << "[VulkanProducer] Slot " << i << " semaphores created" << std::endl;
        }
    }

    void destroyExportableSemaphores() {
        for (uint32_t i = 0; i < producerSemaphores_.size(); i++) {
            if (producerSemaphores_[i] != VK_NULL_HANDLE) {
                vkDestroySemaphore(device_, producerSemaphores_[i], nullptr);
                producerSemaphores_[i] = VK_NULL_HANDLE;
            }
            if (consumerSemaphores_[i] != VK_NULL_HANDLE) {
                vkDestroySemaphore(device_, consumerSemaphores_[i], nullptr);
                consumerSemaphores_[i] = VK_NULL_HANDLE;
            }
            if (semaphoreHandles_[i]) {
                CloseHandle(semaphoreHandles_[i]);
                semaphoreHandles_[i] = nullptr;
            }
        }
    }

    // ── Cross-Process Sync ────────────────────────────────────────

    void waitForConsumer(uint32_t slot) {
        // Timeline semaphore wait: consumer must have finished the previous frame
        uint64_t waitValue = frameCount_; // consumer signals at this value
        VkTimelineSemaphoreSubmitInfo timelineInfo{};
        timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
        timelineInfo.waitSemaphoreValueCount = 1;
        timelineInfo.pWaitSemaphoreValues = &waitValue;

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext = &timelineInfo;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &consumerSemaphores_[slot];
        submitInfo.pWaitDstStageMask = &(VkPipelineStageFlags){ VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };

        VkFence fence;
        VkFenceCreateInfo fenceInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        vkCreateFence(device_, &fenceInfo, nullptr, &fence);

        VkResult res = vkQueueSubmit(graphicsQueue_, 1, &submitInfo, fence);
        if (res == VK_ERROR_DEVICE_LOST) { deviceLost_ = true; return; }
        VK_CHECK(res, "vkQueueSubmit (waitForConsumer)");

        vkWaitForFences(device_, 1, &fence, VK_TRUE, 5'000'000'000); // 5s timeout
        vkDestroyFence(device_, fence, nullptr);
    }

    void signalConsumer(uint32_t slot) {
        uint64_t signalValue = frameCount_;

        VkTimelineSemaphoreSubmitInfo timelineInfo{};
        timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
        timelineInfo.signalSemaphoreValueCount = 1;
        timelineInfo.pSignalSemaphoreValues = &signalValue;

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext = &timelineInfo;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &consumerSemaphores_[slot];

        VkResult res = vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE);
        if (res == VK_ERROR_DEVICE_LOST) { deviceLost_ = true; return; }
        VK_CHECK(res, "vkQueueSubmit (signalConsumer)");
    }

    // ── Config Shared Memory ──────────────────────────────────────

    void createConfigSharedMemory() {
        std::string name = MakeConfigHandleName(config_.instanceName);
        configFileMapping_ = CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            nullptr,
            PAGE_READWRITE,
            0,
            sizeof(SharedConfigBlock),
            name.c_str()
        );
        if (!configFileMapping_) {
            configFileMapping_ = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, name.c_str());
        }
        if (!configFileMapping_) {
            throw std::runtime_error("Failed to create/open config shared memory: " + name);
        }

        configView_ = MapViewOfFile(configFileMapping_, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedConfigBlock));
        if (!configView_) {
            throw std::runtime_error("Failed to map config shared memory view");
        }
    }

    void destroyConfigSharedMemory() {
        if (configView_) { UnmapViewOfFile(configView_); configView_ = nullptr; }
        if (configFileMapping_) { CloseHandle(configFileMapping_); configFileMapping_ = nullptr; }
    }

    void writeConfigBlock(uint32_t activeSlot, uint64_t frameCount) {
        auto* block = reinterpret_cast<SharedConfigBlock*>(configView_);
        block->magic = SHARED_GPU_IMAGE_MAGIC;
        block->version = SHARED_GPU_IMAGE_VERSION;
        block->width = config_.width;
        block->height = config_.height;
        block->format = static_cast<uint32_t>(config_.format);
        block->doubleBufferSlots = config_.doubleBufferSlots;
        block->activeSlot = activeSlot;
        block->frameCount = frameCount;
        block->lastFrameTimeNs = static_cast<uint64_t>(GetTickCount64() * 1'000'000);
        block->producerPID = GetCurrentProcessId();
        block->producerStatus = PRODUCER_FRAME_READY;
        block->flags = 0;

        MemoryBarrier();
    }

    // ── Utilities ─────────────────────────────────────────────────

    VkFormat toVkFormat(SharedImageFormat fmt) {
        switch (fmt) {
            case SharedImageFormat::R8G8B8A8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
            case SharedImageFormat::R8G8B8A8_SRGB: return VK_FORMAT_R8G8B8A8_SRGB;
            case SharedImageFormat::B8G8R8A8_UNORM: return VK_FORMAT_B8G8R8A8_UNORM;
            case SharedImageFormat::B8G8R8A8_SRGB: return VK_FORMAT_B8G8R8A8_SRGB;
            case SharedImageFormat::R16G16B16A16_SFLOAT: return VK_FORMAT_R16G16B16A16_SFLOAT;
            case SharedImageFormat::R16G16B16A16_UNORM: return VK_FORMAT_R16G16B16A16_UNORM;
            case SharedImageFormat::R32G32B32A32_SFLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;
            default: return VK_FORMAT_R8G8B8A8_UNORM;
        }
    }

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags props) {
        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memProps);
        for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & props) == props) {
                return i;
            }
        }
        throw std::runtime_error("Failed to find suitable memory type");
    }
};

// ── C API (exported for N-API / FFI) ───────────────────────────────

extern "C" {

    __declspec(dllexport) VulkanProducer* Producer_Create(uint32_t width, uint32_t height,
        uint32_t format, uint32_t slots, const char* instanceName) {
        try {
            ProducerConfig cfg{};
            cfg.width = width;
            cfg.height = height;
            cfg.format = static_cast<SharedImageFormat>(format);
            cfg.doubleBufferSlots = slots;
            cfg.instanceName = instanceName ? instanceName : "4d-renderer";
            auto* p = new VulkanProducer(cfg);
            p->init();
            return p;
        } catch (const std::exception& e) {
            std::cerr << "[Producer_Create] " << e.what() << std::endl;
            return nullptr;
        }
    }

    __declspec(dllexport) void Producer_Destroy(VulkanProducer* p) {
        delete p;
    }

    __declspec(dllexport) uint32_t Producer_BeginFrame(VulkanProducer* p) {
        return p->beginFrame();
    }

    __declspec(dllexport) void Producer_EndFrame(VulkanProducer* p, uint32_t slot) {
        p->endFrame(slot);
    }

    __declspec(dllexport) void* Producer_GetImageHandle(VulkanProducer* p, uint32_t slot) {
        return p->getImageHandle(slot);
    }

    __declspec(dllexport) void* Producer_GetSemaphoreHandle(VulkanProducer* p, uint32_t slot) {
        return p->getSemaphoreHandle(slot);
    }

    __declspec(dllexport) void Producer_Resize(VulkanProducer* p, uint32_t width, uint32_t height) {
        p->resize(width, height);
    }

    __declspec(dllexport) uint32_t Producer_GetWidth(VulkanProducer* p) { return p->width(); }
    __declspec(dllexport) uint32_t Producer_GetHeight(VulkanProducer* p) { return p->height(); }
    __declspec(dllexport) uint32_t Producer_GetSlot(VulkanProducer* p) { return p->currentSlot(); }
    __declspec(dllexport) uint64_t Producer_GetFrameCount(VulkanProducer* p) { return p->frameCount(); }
    __declspec(dllexport) bool Producer_IsDeviceLost(VulkanProducer* p) { return p->deviceLost(); }
    __declspec(dllexport) SharedConfigBlock* Producer_GetConfig(VulkanProducer* p) { return p->configBlock(); }

} // extern "C"
