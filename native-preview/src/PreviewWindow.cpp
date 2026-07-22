#include "../include/SharedGPUImage.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <stdexcept>
#include <sstream>

static void VK_CHECK(VkResult res, const char* msg) {
    if (res != VK_SUCCESS) {
        std::stringstream ss; ss << msg << ": " << res;
        throw std::runtime_error(ss.str());
    }
}

// ── PreviewWindow ──────────────────────────────────────────────────

class PreviewWindow {
public:
    PreviewWindow(HINSTANCE hInstance, const ConsumerConfig& cfg)
        : config_(cfg)
        , hInstance_(hInstance)
        , hWnd_(nullptr)
        , instance_(VK_NULL_HANDLE)
        , physicalDevice_(VK_NULL_HANDLE)
        , device_(VK_NULL_HANDLE)
        , graphicsQueue_(VK_NULL_HANDLE)
        , presentQueue_(VK_NULL_HANDLE)
        , graphicsQueueFamily_(UINT32_MAX)
        , presentQueueFamily_(UINT32_MAX)
        , surface_(VK_NULL_HANDLE)
        , swapchain_(VK_NULL_HANDLE)
        , configFileMapping_(nullptr)
        , configView_(nullptr)
        , frameIndex_(0)
        , running_(true)
    {
    }

    ~PreviewWindow() {
        shutdown();
    }

    bool init() {
        if (!createWindow()) return false;
        if (!initVulkan()) return false;
        if (!openProducerResources()) return false;
        return true;
    }

    void run() {
        ShowWindow(hWnd_, SW_SHOW);
        UpdateWindow(hWnd_);
        std::cout << "[PreviewWindow] Running. Title: " << config_.windowTitle << std::endl;

        MSG msg = {};
        while (running_ && msg.message != WM_QUIT) {
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            if (running_) {
                presentFrame();
            }
        }
    }

    void shutdown() {
        running_ = false;
        if (device_ != VK_NULL_HANDLE) vkDeviceWaitIdle(device_);
        destroySwapchain();
        destroySurface();
        destroyConfigSharedMemory();
        if (device_ != VK_NULL_HANDLE) {
            vkDestroyDevice(device_, nullptr);
            device_ = VK_NULL_HANDLE;
        }
        if (instance_ != VK_NULL_HANDLE) {
            vkDestroyInstance(instance_, nullptr);
            instance_ = VK_NULL_HANDLE;
        }
        if (hWnd_) { DestroyWindow(hWnd_); hWnd_ = nullptr; }
    }

    void requestResize(uint32_t w, uint32_t h) {
        config_.windowWidth = w;
        config_.windowHeight = h;
        vkDeviceWaitIdle(device_);
        destroySwapchain();
        createSwapchain();
        writeConsumerReady();
    }

private:
    ConsumerConfig config_;
    HINSTANCE hInstance_;
    HWND hWnd_;
    bool running_;

    // Vulkan
    VkInstance instance_;
    VkPhysicalDevice physicalDevice_;
    VkDevice device_;
    VkQueue graphicsQueue_;
    VkQueue presentQueue_;
    uint32_t graphicsQueueFamily_;
    uint32_t presentQueueFamily_;
    VkSurfaceKHR surface_;
    VkSwapchainKHR swapchain_;
    std::vector<VkImage> swapchainImages_;
    std::vector<VkImageView> swapchainViews_;
    VkFormat swapchainFormat_;
    VkExtent2D swapchainExtent_;

    // Imported resources
    HANDLE configFileMapping_ = nullptr;
    void* configView_ = nullptr;
    SharedConfigBlock* sharedConfig_ = nullptr;
    std::vector<VkImage> importedImages_;
    std::vector<VkDeviceMemory> importedMemory_;
    std::vector<VkImageView> importedViews_;
    std::vector<VkSemaphore> importSemaphores_; // consumer timeline semaphores

    uint64_t frameIndex_ = 0;

    // ── Window ────────────────────────────────────────────────────

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        auto* self = reinterpret_cast<PreviewWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
        switch (msg) {
            case WM_CREATE: {
                auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
                SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
                break;
            }
            case WM_SIZE: {
                if (self && wParam != SIZE_MINIMIZED) {
                    uint32_t w = LOWORD(lParam), h = HIWORD(lParam);
                    if (w > 0 && h > 0) self->requestResize(w, h);
                }
                break;
            }
            case WM_DESTROY: {
                if (self) self->running_ = false;
                PostQuitMessage(0);
                break;
            }
            case WM_KEYDOWN: {
                if (wParam == VK_ESCAPE) {
                    if (self) self->running_ = false;
                    PostQuitMessage(0);
                }
                break;
            }
            default: return DefWindowProc(hWnd, msg, wParam, lParam);
        }
        return 0;
    }

    bool createWindow() {
        const char* className = "SXPRPreviewWindow";

        WNDCLASSEXA wc = {};
        wc.cbSize = sizeof(WNDCLASSEXA);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WndProc;
        wc.hInstance = hInstance_;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wc.lpszClassName = className;

        if (!RegisterClassExA(&wc)) {
            std::cerr << "[PreviewWindow] RegisterClass failed" << std::endl;
            return false;
        }

        RECT rect = { 0, 0, static_cast<LONG>(config_.windowWidth), static_cast<LONG>(config_.windowHeight) };
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

        hWnd_ = CreateWindowExA(
            0, className, config_.windowTitle,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            rect.right - rect.left, rect.bottom - rect.top,
            nullptr, nullptr, hInstance_, this
        );

        if (!hWnd_) {
            std::cerr << "[PreviewWindow] CreateWindow failed" << std::endl;
            return false;
        }

        return true;
    }

    // ── Vulkan Init ───────────────────────────────────────────────

    bool initVulkan() {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "4D-Preview";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        const char* extensions[] = {
            VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
            VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME,
            VK_KHR_SURFACE_EXTENSION_NAME,
            "VK_KHR_win32_surface",
        };

        VkInstanceCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        info.pApplicationInfo = &appInfo;
        info.enabledExtensionCount = static_cast<uint32_t>(std::size(extensions));
        info.ppEnabledExtensionNames = extensions;

        VK_CHECK(vkCreateInstance(&info, nullptr, &instance_), "vkCreateInstance");

        // Pick device (same physical device as producer)
        uint32_t count = 0;
        VK_CHECK(vkEnumeratePhysicalDevices(instance_, &count, nullptr), "vkEnumeratePhysicalDevices");
        if (count == 0) throw std::runtime_error("No Vulkan devices");

        std::vector<VkPhysicalDevice> devices(count);
        VK_CHECK(vkEnumeratePhysicalDevices(instance_, &count, devices.data()), "vkEnumeratePhysicalDevices");
        physicalDevice_ = devices[0];

        // Create surface
        VkWin32SurfaceCreateInfoKHR surfInfo{};
        surfInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        surfInfo.hinstance = hInstance_;
        surfInfo.hwnd = hWnd_;
        VK_CHECK(vkCreateWin32SurfaceKHR(instance_, &surfInfo, nullptr, &surface_), "vkCreateWin32SurfaceKHR");

        // Find queue families
        uint32_t qCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &qCount, nullptr);
        std::vector<VkQueueFamilyProperties> queues(qCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &qCount, queues.data());

        for (uint32_t i = 0; i < qCount; i++) {
            if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) graphicsQueueFamily_ = i;
            VkBool32 present = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice_, i, surface_, &present);
            if (present) presentQueueFamily_ = i;
        }

        // Create device with external memory/semaphore support
        const float queuePriority = 1.0f;
        std::vector<VkDeviceQueueCreateInfo> queueInfos;
        uint32_t uniqueFamilies[] = { graphicsQueueFamily_, presentQueueFamily_ };
        for (auto f : uniqueFamilies) {
            VkDeviceQueueCreateInfo qInfo{};
            qInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            qInfo.queueFamilyIndex = f;
            qInfo.queueCount = 1;
            qInfo.pQueuePriorities = &queuePriority;
            queueInfos.push_back(qInfo);
        }

        const char* devExtensions[] = {
            VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME,
            VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
            VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME,
            VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME,
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };

        VkDeviceCreateInfo devInfo{};
        devInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        devInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
        devInfo.pQueueCreateInfos = queueInfos.data();
        devInfo.enabledExtensionCount = static_cast<uint32_t>(std::size(devExtensions));
        devInfo.ppEnabledExtensionNames = devExtensions;

        VK_CHECK(vkCreateDevice(physicalDevice_, &devInfo, nullptr, &device_), "vkCreateDevice");
        vkGetDeviceQueue(device_, graphicsQueueFamily_, 0, &graphicsQueue_);
        vkGetDeviceQueue(device_, presentQueueFamily_, 0, &presentQueue_);

        createSwapchain();

        return true;
    }

    void createSwapchain() {
        VkSurfaceCapabilitiesKHR caps;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice_, surface_, &caps);

        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_, &formatCount, nullptr);
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_, &formatCount, formats.data());
        swapchainFormat_ = formats[0].format;

        swapchainExtent_ = caps.currentExtent;
        if (swapchainExtent_.width == UINT32_MAX) {
            swapchainExtent_ = { config_.windowWidth, config_.windowHeight };
        }

        VkSwapchainCreateInfoKHR info{};
        info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        info.surface = surface_;
        info.minImageCount = std::max(2u, caps.minImageCount);
        info.imageFormat = swapchainFormat_;
        info.imageExtent = swapchainExtent_;
        info.imageArrayLayers = 1;
        info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        info.preTransform = caps.currentTransform;
        info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        info.presentMode = config_.enableVsync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;
        info.clipped = VK_TRUE;

        VK_CHECK(vkCreateSwapchainKHR(device_, &info, nullptr, &swapchain_), "vkCreateSwapchainKHR");

        // Get swapchain images
        uint32_t imgCount = 0;
        vkGetSwapchainImagesKHR(device_, swapchain_, &imgCount, nullptr);
        swapchainImages_.resize(imgCount);
        swapchainViews_.resize(imgCount);
        vkGetSwapchainImagesKHR(device_, swapchain_, &imgCount, swapchainImages_.data());

        for (uint32_t i = 0; i < imgCount; i++) {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = swapchainImages_[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = swapchainFormat_;
            viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
            VK_CHECK(vkCreateImageView(device_, &viewInfo, nullptr, &swapchainViews_[i]), "vkCreateImageView");
        }

        config_.windowWidth = swapchainExtent_.width;
        config_.windowHeight = swapchainExtent_.height;
    }

    void destroySwapchain() {
        for (auto v : swapchainViews_) vkDestroyImageView(device_, v, nullptr);
        swapchainViews_.clear();
        if (swapchain_) { vkDestroySwapchainKHR(device_, swapchain_, nullptr); swapchain_ = VK_NULL_HANDLE; }
        swapchainImages_.clear();
    }

    void destroySurface() {
        if (surface_) { vkDestroySurfaceKHR(instance_, surface_, nullptr); surface_ = VK_NULL_HANDLE; }
    }

    // ── Import Producer Resources ─────────────────────────────────

    bool openProducerResources() {
        // Open config shared memory
        std::string configName = MakeConfigHandleName(config_.instanceName);
        configFileMapping_ = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, configName.c_str());
        if (!configFileMapping_) {
            std::cerr << "[PreviewWindow] Config not found. Is the producer running?" << std::endl;
            return false;
        }

        configView_ = MapViewOfFile(configFileMapping_, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedConfigBlock));
        if (!configView_) {
            std::cerr << "[PreviewWindow] Failed to map config view" << std::endl;
            return false;
        }

        sharedConfig_ = reinterpret_cast<SharedConfigBlock*>(configView_);

        // Read initial config
        uint32_t width = sharedConfig_->width;
        uint32_t height = sharedConfig_->height;
        uint32_t slots = sharedConfig_->doubleBufferSlots;
        VkFormat vkFormat = toVkFormat(static_cast<SharedImageFormat>(sharedConfig_->format));

        std::cout << "[PreviewWindow] Producer config: "
                  << width << "x" << height
                  << " slots=" << slots << std::endl;

        importedImages_.resize(slots);
        importedMemory_.resize(slots);
        importedViews_.resize(slots);
        importSemaphores_.resize(slots);

        auto vkGetMemoryWin32HandleKHR = reinterpret_cast<PFN_vkGetMemoryWin32HandleKHR>(
            vkGetDeviceProcAddr(device_, "vkGetMemoryWin32HandleKHR"));

        for (uint32_t i = 0; i < slots; i++) {
            // Open the named image handle
            std::string imageName = MakeImageHandleName(config_.instanceName, i);
            HANDLE imageHandle = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, imageName.c_str());
            if (!imageHandle) {
                std::cerr << "[PreviewWindow] Failed to open image handle for slot " << i << std::endl;
                continue;
            }

            VkMemoryGetWin32HandleInfoKHR getInfo{};
            getInfo.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
            getInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
            getInfo.memory = reinterpret_cast<VkDeviceMemory>(imageHandle); // placeholder

            // Import memory
            VkImportMemoryWin32HandleInfoKHR importInfo{};
            importInfo.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR;
            importInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
            importInfo.handle = imageHandle;

            VkMemoryRequirements memReqs{};
            memReqs.size = width * height * 4; // RGBA8
            memReqs.memoryTypeBits = 0xFFFFFFFF;

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.pNext = &importInfo;
            allocInfo.allocationSize = memReqs.size;
            allocInfo.memoryTypeIndex = 0; // must match producer

            VkResult res = vkAllocateMemory(device_, &allocInfo, nullptr, &importedMemory_[i]);
            if (res != VK_SUCCESS) {
                std::cerr << "[PreviewWindow] Failed to import memory for slot " << i << std::endl;
                CloseHandle(imageHandle);
                continue;
            }

            // Create image wrapping imported memory
            VkImageCreateInfo imgInfo{};
            imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imgInfo.imageType = VK_IMAGE_TYPE_2D;
            imgInfo.format = vkFormat;
            imgInfo.extent = { width, height, 1 };
            imgInfo.mipLevels = 1;
            imgInfo.arrayLayers = 1;
            imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imgInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            VK_CHECK(vkCreateImage(device_, &imgInfo, nullptr, &importedImages_[i]), "vkCreateImage (import)");
            VK_CHECK(vkBindImageMemory(device_, importedImages_[i], importedMemory_[i], 0), "vkBindImageMemory (import)");

            // Image view
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = importedImages_[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = vkFormat;
            viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
            VK_CHECK(vkCreateImageView(device_, &viewInfo, nullptr, &importedViews_[i]), "vkCreateImageView (import)");

            // Import semaphore
            std::string semName = MakeSemaphoreHandleName(config_.instanceName, i);
            HANDLE semHandle = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, semName.c_str());
            if (semHandle) {
                VkSemaphoreTypeCreateInfo typeInfo{};
                typeInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
                typeInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;

                VkImportSemaphoreWin32HandleInfoKHR semImport{};
                semImport.sType = VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_WIN32_HANDLE_INFO_KHR;
                semImport.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
                semImport.handle = semHandle;

                VkSemaphoreCreateInfo semInfo{};
                semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
                semInfo.pNext = &semImport;

                VK_CHECK(vkCreateSemaphore(device_, &semInfo, nullptr, &importSemaphores_[i]), "vkCreateSemaphore (import)");
            }

            CloseHandle(imageHandle);
            if (semHandle) CloseHandle(semHandle);

            std::cout << "[PreviewWindow] Imported slot " << i << std::endl;
        }

        writeConsumerReady();
        return true;
    }

    void destroyConfigSharedMemory() {
        if (configView_) { UnmapViewOfFile(configView_); configView_ = nullptr; }
        if (configFileMapping_) { CloseHandle(configFileMapping_); configFileMapping_ = nullptr; }

        for (size_t i = 0; i < importedViews_.size(); i++) {
            if (importedViews_[i]) vkDestroyImageView(device_, importedViews_[i], nullptr);
        }
        for (size_t i = 0; i < importedImages_.size(); i++) {
            if (importedImages_[i]) vkDestroyImage(device_, importedImages_[i], nullptr);
        }
        for (size_t i = 0; i < importedMemory_.size(); i++) {
            if (importedMemory_[i]) vkFreeMemory(device_, importedMemory_[i], nullptr);
        }
        for (size_t i = 0; i < importSemaphores_.size(); i++) {
            if (importSemaphores_[i]) vkDestroySemaphore(device_, importSemaphores_[i], nullptr);
        }
    }

    // ── Frame Presentation ────────────────────────────────────────

    void presentFrame() {
        if (!sharedConfig_) return;

        // Read latest config
        uint32_t activeSlot = sharedConfig_->activeSlot;
        uint64_t producerFrame = sharedConfig_->frameCount;

        if (producerFrame <= frameIndex_) {
            Sleep(1); // No new frame yet
            return;
        }
        frameIndex_ = producerFrame;

        if (activeSlot >= importedImages_.size() || !importedImages_[activeSlot]) return;

        // Acquire swapchain image
        uint32_t imageIndex;
        VkResult aquireRes = vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX,
            VK_NULL_HANDLE, VK_NULL_HANDLE, &imageIndex);
        if (aquireRes == VK_ERROR_OUT_OF_DATE_KHR) {
            requestResize(config_.windowWidth, config_.windowHeight);
            return;
        }

        // Blit imported image to swapchain
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = graphicsQueueFamily_;

        VkCommandPool cmdPool;
        VK_CHECK(vkCreateCommandPool(device_, &poolInfo, nullptr, &cmdPool), "vkCreateCommandPool");

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = cmdPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer cmd;
        VK_CHECK(vkAllocateCommandBuffers(device_, &allocInfo, &cmd), "vkAllocateCommandBuffers");

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo), "vkBeginCommandBuffer");

        // Transition imported image to TRANSFER_SRC_OPTIMAL
        VkImageMemoryBarrier barrierSrc{};
        barrierSrc.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrierSrc.srcAccessMask = 0;
        barrierSrc.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrierSrc.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrierSrc.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrierSrc.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrierSrc.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrierSrc.image = importedImages_[activeSlot];
        barrierSrc.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrierSrc);

        // Transition swapchain image to TRANSFER_DST_OPTIMAL
        VkImageMemoryBarrier barrierDst{};
        barrierDst.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrierDst.srcAccessMask = 0;
        barrierDst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrierDst.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrierDst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrierDst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrierDst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrierDst.image = swapchainImages_[imageIndex];
        barrierDst.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrierDst);

        // Blit
        VkImageBlit blit{};
        blit.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { static_cast<int32_t>(sharedConfig_->width),
                               static_cast<int32_t>(sharedConfig_->height), 1 };
        blit.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { static_cast<int32_t>(swapchainExtent_.width),
                               static_cast<int32_t>(swapchainExtent_.height), 1 };

        vkCmdBlitImage(cmd, importedImages_[activeSlot], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            swapchainImages_[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit, VK_FILTER_LINEAR);

        // Transition swapchain to PRESENT_SRC
        VkImageMemoryBarrier barrierPresent{};
        barrierPresent.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrierPresent.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrierPresent.dstAccessMask = 0;
        barrierPresent.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrierPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrierPresent.image = swapchainImages_[imageIndex];
        barrierPresent.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrierPresent);

        VK_CHECK(vkEndCommandBuffer(cmd), "vkEndCommandBuffer");

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;

        VkResult submitRes = vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE);
        if (submitRes == VK_ERROR_DEVICE_LOST) {
            std::cerr << "[PreviewWindow] Device lost during present" << std::endl;
            return;
        }

        vkQueueWaitIdle(graphicsQueue_);
        vkDestroyCommandPool(device_, cmdPool, nullptr);

        // Present
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain_;
        presentInfo.pImageIndices = &imageIndex;

        VkResult presentRes = vkQueuePresentKHR(presentQueue_, &presentInfo);
        if (presentRes == VK_ERROR_OUT_OF_DATE_KHR || presentRes == VK_SUBOPTIMAL_KHR) {
            requestResize(config_.windowWidth, config_.windowHeight);
        }
    }

    void writeConsumerReady() {
        if (!sharedConfig_) return;
        sharedConfig_->consumerPID = GetCurrentProcessId();
        sharedConfig_->consumerStatus = CONSUMER_CONNECTED;
        MemoryBarrier();
    }

    VkFormat toVkFormat(SharedImageFormat fmt) {
        switch (fmt) {
            case SharedImageFormat::R8G8B8A8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
            case SharedImageFormat::B8G8R8A8_UNORM: return VK_FORMAT_B8G8R8A8_UNORM;
            case SharedImageFormat::R16G16B16A16_SFLOAT: return VK_FORMAT_R16G16B16A16_SFLOAT;
            default: return VK_FORMAT_R8G8B8A8_UNORM;
        }
    }
};

// ── Entry Point ─────────────────────────────────────────────────────

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, int nCmdShow) {
    try {
        ConsumerConfig cfg{};
        cfg.windowWidth = 1280;
        cfg.windowHeight = 720;
        cfg.windowTitle = "4D Renderer - Native GPU Preview";
        cfg.useDXGIInterop = false;
        cfg.enableVsync = true;
        cfg.instanceName = "4d-renderer";

        // Parse command line
        if (__argc > 1) {
            for (int i = 1; i < __argc; i++) {
                if (strcmp(__argv[i], "--width") == 0 && i + 1 < __argc)
                    cfg.windowWidth = static_cast<uint32_t>(atoi(__argv[++i]));
                else if (strcmp(__argv[i], "--height") == 0 && i + 1 < __argc)
                    cfg.windowHeight = static_cast<uint32_t>(atoi(__argv[++i]));
                else if (strcmp(__argv[i], "--title") == 0 && i + 1 < __argc)
                    cfg.windowTitle = __argv[++i];
                else if (strcmp(__argv[i], "--instance") == 0 && i + 1 < __argc)
                    cfg.instanceName = __argv[++i];
                else if (strcmp(__argv[i], "--dxgi") == 0)
                    cfg.useDXGIInterop = true;
                else if (strcmp(__argv[i], "--no-vsync") == 0)
                    cfg.enableVsync = false;
            }
        }

        PreviewWindow window(hInstance, cfg);
        if (!window.init()) {
            std::cerr << "Failed to initialize preview window" << std::endl;
            return 1;
        }
        window.run();
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << std::endl;
        MessageBoxA(nullptr, e.what(), "Preview Error", MB_ICONERROR);
        return 1;
    }
}
