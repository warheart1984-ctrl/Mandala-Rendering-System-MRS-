#include "../include/SharedGPUImage.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <cstring>
#include <stdexcept>

// ── Handle Manager ──────────────────────────────────────────────────
// Manages the lifecycle of named Win32 handles across processes.
// Handles cleanup on crash, device loss, and graceful restart.

class HandleManager {
public:
    HandleManager(const char* instanceName)
        : instanceName_(instanceName ? instanceName : "4d-renderer")
        , watcherRunning_(false)
        , deviceLost_(false)
    {
    }

    ~HandleManager() {
        stopWatcher();
        cleanupAll();
    }

    // ── Named Handle Creation ─────────────────────────────────────

    HANDLE createOrOpenImageHandle(uint32_t slot, size_t size) {
        std::string name = MakeImageHandleName(instanceName_.c_str(), slot);
        HANDLE h = CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            nullptr,
            PAGE_READWRITE,
            0,
            static_cast<DWORD>(size),
            name.c_str()
        );
        if (!h) {
            // Producer may have already created it — open existing
            h = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, name.c_str());
        }
        if (h) {
            std::lock_guard<std::mutex> lock(mutex_);
            handles_.push_back({ name, h, HandleType::IMAGE, slot });
        }
        return h;
    }

    HANDLE createOrOpenSemaphoreHandle(uint32_t slot) {
        std::string name = MakeSemaphoreHandleName(instanceName_.c_str(), slot);
        HANDLE h = CreateSemaphoreA(nullptr, 0, 1, name.c_str());
        if (!h) {
            h = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, FALSE, name.c_str());
        }
        if (h) {
            std::lock_guard<std::mutex> lock(mutex_);
            handles_.push_back({ name, h, HandleType::SEMAPHORE, slot });
        }
        return h;
    }

    HANDLE createOrOpenConfigHandle() {
        std::string name = MakeConfigHandleName(instanceName_.c_str());
        HANDLE h = CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            nullptr,
            PAGE_READWRITE,
            0,
            sizeof(SharedConfigBlock),
            name.c_str()
        );
        if (!h) {
            h = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, name.c_str());
        }
        if (h) {
            std::lock_guard<std::mutex> lock(mutex_);
            handles_.push_back({ name, h, HandleType::CONFIG, UINT32_MAX });
        }
        return h;
    }

    // ── Cleanup ───────────────────────────────────────────────────

    void closeHandle(HANDLE h) {
        if (h) {
            CloseHandle(h);
            std::lock_guard<std::mutex> lock(mutex_);
            handles_.erase(
                std::remove_if(handles_.begin(), handles_.end(),
                    [h](const HandleEntry& e) { return e.handle == h; }),
                handles_.end()
            );
        }
    }

    void cleanupSlot(uint32_t slot) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto it = handles_.begin(); it != handles_.end();) {
            if (it->slot == slot || it->slot == UINT32_MAX) {
                CloseHandle(it->handle);
                it = handles_.erase(it);
            } else {
                ++it;
            }
        }
    }

    void cleanupAll() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& entry : handles_) {
            CloseHandle(entry.handle);
        }
        handles_.clear();
    }

    // ── Device Loss Detection ─────────────────────────────────────

    void setDeviceLost(bool lost) { deviceLost_ = lost; }
    bool isDeviceLost() const { return deviceLost_; }

    void startWatcher(std::function<void()> onDeviceLost, std::function<void()> onCrash) {
        watcherRunning_ = true;
        watcherThread_ = std::thread([this, onDeviceLost, onCrash]() {
            while (watcherRunning_) {
                // Check device loss flag
                if (deviceLost_) {
                    if (onDeviceLost) onDeviceLost();
                    deviceLost_ = false;
                }

                // Check if the config handle is still valid
                HANDLE configHandle = nullptr;
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    for (const auto& entry : handles_) {
                        if (entry.type == HandleType::CONFIG) {
                            configHandle = entry.handle;
                            break;
                        }
                    }
                }

                if (configHandle) {
                    DWORD waitResult = WaitForSingleObject(configHandle, 0);
                    if (waitResult == WAIT_FAILED) {
                        if (onCrash) onCrash();
                    }
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });
    }

    void stopWatcher() {
        watcherRunning_ = false;
        if (watcherThread_.joinable()) watcherThread_.join();
    }

    // ── Daemon Restart ────────────────────────────────────────────

    bool launchPreviewProcess(const char* executablePath, const ConsumerConfig& config) {
        std::string cmdLine = std::string("\"") + executablePath + "\""
            + " --instance " + instanceName_
            + " --width " + std::to_string(config.windowWidth)
            + " --height " + std::to_string(config.windowHeight);

        STARTUPINFOA si = { sizeof(si) };
        PROCESS_INFORMATION pi = {};

        if (!CreateProcessA(
            executablePath, &cmdLine[0],
            nullptr, nullptr, FALSE,
            CREATE_NO_WINDOW,
            nullptr, nullptr, &si, &pi
        )) {
            DWORD err = GetLastError();
            std::cerr << "[HandleManager] Launch preview failed: error " << err << std::endl;
            return false;
        }

        {
            std::lock_guard<std::mutex> lock(mutex_);
            previewProcess_ = pi.hProcess;
            previewThread_ = pi.hThread;
            previewPID_ = pi.dwProcessId;
        }

        std::cout << "[HandleManager] Preview process launched: PID " << pi.dwProcessId << std::endl;
        CloseHandle(pi.hThread);
        return true;
    }

    void terminatePreview() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (previewProcess_) {
            TerminateProcess(previewProcess_, 0);
            WaitForSingleObject(previewProcess_, 5000);
            CloseHandle(previewProcess_);
            previewProcess_ = nullptr;
            previewPID_ = 0;
        }
    }

    DWORD getPreviewPID() const { return previewPID_; }
    bool isPreviewRunning() const {
        DWORD exitCode = 0;
        if (!GetExitCodeProcess(previewProcess_, &exitCode)) return false;
        return exitCode == STILL_ACTIVE;
    }

    // ── Recovery ──────────────────────────────────────────────────

    bool recoverFromDeviceLoss() {
        std::cout << "[HandleManager] Attempting device-loss recovery..." << std::endl;
        deviceLost_ = false;
        cleanupAll();
        return true;
    }

    bool recoverFromCrash() {
        std::cout << "[HandleManager] Attempting crash recovery..." << std::endl;
        terminatePreview();
        cleanupAll();
        return true;
    }

private:
    enum class HandleType { IMAGE, SEMAPHORE, CONFIG };

    struct HandleEntry {
        std::string name;
        HANDLE handle;
        HandleType type;
        uint32_t slot;
    };

    std::string instanceName_;
    std::vector<HandleEntry> handles_;
    std::mutex mutex_;
    HANDLE previewProcess_ = nullptr;
    HANDLE previewThread_ = nullptr;
    DWORD previewPID_ = 0;
    std::thread watcherThread_;
    std::atomic<bool> watcherRunning_;
    std::atomic<bool> deviceLost_;
};

// ── C API ──────────────────────────────────────────────────────────

extern "C" {

    __declspec(dllexport) HandleManager* Sync_Create(const char* instanceName) {
        return new HandleManager(instanceName);
    }

    __declspec(dllexport) void Sync_Destroy(HandleManager* mgr) {
        delete mgr;
    }

    __declspec(dllexport) HANDLE Sync_CreateImageHandle(HandleManager* mgr, uint32_t slot, size_t size) {
        return mgr->createOrOpenImageHandle(slot, size);
    }

    __declspec(dllexport) HANDLE Sync_CreateSemaphoreHandle(HandleManager* mgr, uint32_t slot) {
        return mgr->createOrOpenSemaphoreHandle(slot);
    }

    __declspec(dllexport) HANDLE Sync_CreateConfigHandle(HandleManager* mgr) {
        return mgr->createOrOpenConfigHandle();
    }

    __declspec(dllexport) void Sync_CleanupSlot(HandleManager* mgr, uint32_t slot) {
        mgr->cleanupSlot(slot);
    }

    __declspec(dllexport) void Sync_CleanupAll(HandleManager* mgr) {
        mgr->cleanupAll();
    }

    __declspec(dllexport) void Sync_SetDeviceLost(HandleManager* mgr, bool lost) {
        mgr->setDeviceLost(lost);
    }

    __declspec(dllexport) bool Sync_IsDeviceLost(HandleManager* mgr) {
        return mgr->isDeviceLost();
    }

    __declspec(dllexport) bool Sync_LaunchPreview(HandleManager* mgr,
        const char* exePath, uint32_t width, uint32_t height, bool vsync) {
        ConsumerConfig cfg{};
        cfg.windowWidth = width;
        cfg.windowHeight = height;
        cfg.enableVsync = vsync;
        return mgr->launchPreviewProcess(exePath, cfg);
    }

    __declspec(dllexport) void Sync_TerminatePreview(HandleManager* mgr) {
        mgr->terminatePreview();
    }

    __declspec(dllexport) bool Sync_RecoverDeviceLoss(HandleManager* mgr) {
        return mgr->recoverFromDeviceLoss();
    }

    __declspec(dllexport) bool Sync_RecoverCrash(HandleManager* mgr) {
        return mgr->recoverFromCrash();
    }

} // extern "C"
