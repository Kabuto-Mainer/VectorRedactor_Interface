#ifndef APPLICATION_CORE_HPP
#define APPLICATION_CORE_HPP

#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <dlfcn.h>
#include <iostream>

#include "SDK/Interfaces/IUnknown.hpp"
#include "SDK/Interfaces/IPlugin.hpp"
#include "SDK/Utils/AObject.hpp"

// NOTE: Forward declaration
class IRender;
class IInputPump;
class ILogger;
struct Event;

class ApplicationCore {
private:
    IRender*    render_ = nullptr;
    IInputPump* input_  = nullptr;
    ILogger*    logger_ = nullptr;

    std::unordered_map<IID, IPlugin*> registry_;

    std::unique_ptr<AObject> root_;

    std::vector<void*> loaded_handles_;

    bool is_running_ = false;

public:
    // -------------------------------------------------------------------------------
    // --- Constructor ---

    ApplicationCore() {
        root_ = std::make_unique<AObject>();
    }

    // --- Destructor ---

    ~ApplicationCore() {
        std::cout << "[Core] Shutting down application and clearing resources...\n";

        if (render_) render_->release();
        if (input_)  input_->release();
        if (logger_) logger_->release();

        for (auto& [iid, service] : registry_) {
            if (service) service->release();
        }
        registry_.clear();

        for (auto it = loaded_handles_.rbegin(); it != loaded_handles_.rend(); ++it) {
            if (*it) dlclose(*it);
        }
        loaded_handles_.clear();
    }

    // --- Copy Semantics Disabled ---

    ApplicationCore(const ApplicationCore&) = delete;
    ApplicationCore& operator=(const ApplicationCore&) = delete;

    // -------------------------------------------------------------------------------
    // --- Getters & Setters ---

    static ApplicationCore& instance() { static ApplicationCore core; return core; }

    AObject*    getRoot()      const { return root_.get(); }

    // -------------------------------------------------------------------------------

    void stop() { is_running_ = false; }

    HRESULT getInterface(IID iid, void** ppv) {
        if (!ppv) return E_INVALIDARG;
        auto it = registry_.find(iid);
        if (it != registry_.end() && it->second != nullptr) {
            return it->second->getInterface(iid, ppv);
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    HRESULT registerInterface(IID iid, IPlugin* service) {
        if (!service) return E_INVALIDARG;
        if (registry_.find(iid) != registry_.end()) {
            return E_FAIL;
        }
        service->addRef();
        registry_[iid] = service;
        return S_OK;
    }

    bool loadPlugin(const std::string& path) {
        void* handle = dlopen(path.c_str(), RTLD_NOW);
        if (!handle) {
            std::cerr << "[Core] dlopen error: " << dlerror() << "\n";
            return false;
        }

        using GetPluginFunc = IUnknown* (*)();
        auto getPluginInstance = (GetPluginFunc)dlsym(handle, "getInterface");

        if (!getPluginInstance) {
            std::cerr << "[Core] Missing 'getInterface' function in " << path << "\n";
            dlclose(handle);
            return false;
        }

        IUnknown* raw_plugin = getPluginInstance();
        if (!raw_plugin) {
            dlclose(handle);
            return false;
        }

        IPlugin* plugin = nullptr;
        constexpr IID IID_IPlugin = 0x0002;

        void* pv_plugin = nullptr;
        if (raw_plugin->getInterface(IID_IPlugin, &pv_plugin) == S_OK) {
            plugin = static_cast<IPlugin*>(pv_plugin);
            if (plugin->init() != S_OK) {
                std::cerr << "[Core] Plugin init failed: " << path << "\n";
                plugin->release(); raw_plugin->release(); dlclose(handle); return false;
            }
            plugin->release();
        } else {
            raw_plugin->release(); dlclose(handle); return false;
        }

        constexpr IID IID_IRender    = 0x0003;
        constexpr IID IID_IInputPump = 0x0004;
        constexpr IID IID_ILogger    = 0x0005;

        void* pv_render = nullptr;
        void* pv_input  = nullptr;
        void* pv_logger = nullptr;

        raw_plugin->getInterface(IID_IRender,    &pv_render);
        raw_plugin->getInterface(IID_IInputPump, &pv_input);
        raw_plugin->getInterface(IID_ILogger,    &pv_logger);

        render_ = static_cast<IRender*>(pv_render);
        input_  = static_cast<IInputPump*>(pv_input);
        logger_ = static_cast<ILogger*>(pv_logger);

        if (!render_ || !input_) {
            std::cerr << "[Core] Loaded plugin missing critical IRender or IInputPump\n";
            if (render_) { render_->release(); render_ = nullptr; }
            if (input_)  { input_->release();  input_ = nullptr;  }
            if (logger_) { logger_->release(); logger_ = nullptr; }
            raw_plugin->release();
            dlclose(handle);
            return false;
        }

        loaded_handles_.push_back(handle);
        raw_plugin->release();
        return true;
    }

    // -------------------------------------------------------------------------------
    // --- Main Loop ---

    int run() {
        if (!render_ || !input_) {
            std::cerr << "[Core] Subsystems not initialized. Call loadPlugin first.\n";
            return -1;
        }

        std::cout << "[Core] Entering hardware-accelerated main loop...\n";
        is_running_ = true;

        while (is_running_) {
            Event event;

            // Input polling
            while (input_->pollEvent(event)) {
                if (event.type == 67) { // Let's say 67 means Quit.
                    is_running_ = false;
                    break;
                }
                root_->processEvent(event);
            }

            // Rendering the object tree
            render_->beginFrame();

            root_->render();

            render_->endFrame();
        }

        std::cout << "[Core] Exited main loop.\n";
        return 0;
    }
};

#endif
