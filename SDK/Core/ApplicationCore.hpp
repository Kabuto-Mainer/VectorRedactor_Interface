#ifndef APPLICATION_CORE_HPP
#define APPLICATION_CORE_HPP

#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <dlfcn.h>
#include <iostream>

#include "Interfaces/IUnknown.hpp"
#include "Interfaces/IPlugin.hpp"
#include "Utils/AObject.hpp"

constexpr IID IID_IUnknown   = 0x0001;
constexpr IID IID_IPlugin    = 0x0002; 
constexpr IID IID_IRender    = 0x0003;
constexpr IID IID_IInputPump = 0x0004;
constexpr IID IID_ILogger    = 0x0005;

// NOTE: Forward declaration
struct Event;

class IRender;
class IInputPump;
class ILogger;

class ApplicationCore {
private:
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
    
    AObject* getRoot() const { return root_.get(); }
    
    void stop() { is_running_ = false; }

    // -------------------------------------------------------------------------------

    HRESULT registerInterface(IID iid, IPlugin* service) {
        if (!service) return E_INVALIDARG;
        if (registry_.find(iid) != registry_.end()) {
            return E_FAIL; 
        }
        service->addRef(); 
        registry_[iid] = service;
        return S_OK;
    }
    
    HRESULT getInterface(IID iid, void** ppv) {
        if (!ppv) return E_INVALIDARG;
        *ppv = nullptr;

        auto it = registry_.find(iid);
        if (it != registry_.end() && it->second != nullptr) {
            return it->second->getInterface(iid, ppv);
        }

        return E_NOINTERFACE;
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

        void* pv_plugin = nullptr;
        if (raw_plugin->getInterface(IID_IPlugin, &pv_plugin) == S_OK) {
            auto* plugin = static_cast<IPlugin*>(pv_plugin);
            if (plugin->init() != S_OK) {
                std::cerr << "[Core] Plugin init failed: " << path << "\n";
                plugin->release(); raw_plugin->release(); dlclose(handle); return false;
            }
            registerInterface(IID_IPlugin, plugin);
            plugin->release();
        } else {
            raw_plugin->release(); dlclose(handle); return false;
        }

        loaded_handles_.push_back(handle);
        raw_plugin->release(); 
        return true;
    }

    // -------------------------------------------------------------------------------
    // --- Main Loop ---

    int run() {
        IRender* render = nullptr;
        IInputPump* input = nullptr;

        if (getInterface(IID_IRender, (void**)&render) != S_OK ||
            getInterface(IID_IInputPump, (void**)&input) != S_OK) {
            std::cerr << "[Core] Critical subsystems missing in registry. Call loadPlugin first.\n";
            if (render) render->release();
            return -1;
        }

        std::cout << "[Core] Entering hardware-accelerated main loop...\n";
        is_running_ = true;

        while (is_running_) {
            Event event;

            // Input polling
            while (input->pollEvent(event)) {
                if (event.type == 67) { // Let's say 67 means Quit.
                    is_running = false;
                    break;
                }
                root_->processEvent(event);
            }

            // Rendering the object tree
            render->beginFrame();
            root_->render();
            render->endFrame();
        }

        render->release();
        input->release();

        std::cout << "[Core] Exited main loop.\n";
        return 0;
    }
};

#endif