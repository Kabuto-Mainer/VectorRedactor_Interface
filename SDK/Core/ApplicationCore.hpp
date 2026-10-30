#ifndef APPLICATION_CORE_HPP
#define APPLICATION_CORE_HPP

#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IPlugin.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/IUnknown.hpp"
#include "Utils/AObject.hpp"

#include <cassert>
#include <dlfcn.h>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

constexpr IID IID_UNKNOWN    = 0x00'01;
constexpr IID IID_PLUGIN     = 0x00'02;
constexpr IID IID_RENDERER   = 0x00'03;
constexpr IID IID_INPUT_PUMP = 0x00'04;
constexpr IID IID_LOGGER     = 0x00'05;

// NOTE: Forward declaration
struct Event;

class ApplicationCore
{
private:
    IRenderer*                        renderer_   = nullptr;
    IInputPump*                       input_pump_ = nullptr;
    std::unordered_map<IID, IPlugin*> registry_;
    std::unique_ptr<AObject>          root_;
    std::vector<void*>                loaded_handles_;
    bool                              is_running_ = false;

public:
    ApplicationCore();
    ~ApplicationCore();

    ApplicationCore(const ApplicationCore&)            = delete;
    ApplicationCore(ApplicationCore&&)                 = delete;
    ApplicationCore& operator=(const ApplicationCore&) = delete;
    ApplicationCore& operator=(ApplicationCore&&)      = delete;

    AObject*    root() const;
    IRenderer*  renderer() const;
    IInputPump* inputPump() const;

    int  run();
    void stop();

    HRESULT                 registerPluginInterface(IID iid, IPlugin* service);
    std::optional<IPlugin*> getPluginInterface(IID iid);
    bool                    loadPlugin(const std::string& path);
};

/* ======================== Implementation ======================== */

ApplicationCore::ApplicationCore()
{
    root_ = std::make_unique<AObject>();
}

ApplicationCore::~ApplicationCore()
{
    for (auto& [iid, service] : registry_)
    {
        if (service != nullptr)
        {
            service->release();
        }
    }
    registry_.clear();

    for (auto it = loaded_handles_.rbegin(); it != loaded_handles_.rend(); ++it)
    {
        if (*it != nullptr)
        {
            dlclose(*it);
        }
    }
    loaded_handles_.clear();
}

AObject* ApplicationCore::root() const
{
    return root_.get();
}

IRenderer* ApplicationCore::renderer() const
{
    assert(renderer_);
    return renderer_;
}

IInputPump* ApplicationCore::inputPump() const
{
    assert(input_pump_);
    return input_pump_;
}

void ApplicationCore::stop()
{
    is_running_ = false;
}

HRESULT ApplicationCore::registerPluginInterface(IID iid, IPlugin* service)
{
    if (service == nullptr)
    {
        return E_INVALIDARG;
    }
    if (registry_.contains(iid))
    {
        return E_FAIL;
    }
    service->addRef();
    registry_[iid] = service;
    return S_OK;
}

std::optional<IPlugin*> ApplicationCore::getPluginInterface(IID iid)
{
    IPlugin* plugin = nullptr;
    auto     found  = registry_.find(iid);

    if (registry_.contains(iid))
    {
        found->second->queryInterface(iid, reinterpret_cast<void**>(&plugin));
        return plugin;
    }

    return std::nullopt;
}

bool ApplicationCore::loadPlugin(const std::string& path)
{
    using GetPluginFunc = IUnknown* (*)();

    void* handle = dlopen(path.c_str(), RTLD_NOW);
    if (handle == nullptr)
    {
        return false;
    }

    auto get_plugin_instance = (GetPluginFunc)dlsym(handle, "getInterface");

    if (get_plugin_instance == nullptr)
    {
        dlclose(handle);
        return false;
    }

    IUnknown* raw_plugin = get_plugin_instance();
    if (raw_plugin == nullptr)
    {
        dlclose(handle);
        return false;
    }

    void* pv_plugin = nullptr;
    if (raw_plugin->queryInterface(IID_PLUGIN, &pv_plugin) == S_OK)
    {
        auto* plugin = static_cast<IPlugin*>(pv_plugin);
        if (plugin->init(this) != S_OK)
        {
            plugin->release();
            raw_plugin->release();
            dlclose(handle);
            return false;
        }

        registerPluginInterface(IID_PLUGIN, plugin);
        plugin->release();
    }
    else
    {
        raw_plugin->release();
        dlclose(handle);
        return false;
    }

    loaded_handles_.push_back(handle);
    raw_plugin->release();
    return true;
}

int ApplicationCore::run()
{
    /* already running => return */
    if (is_running_)
    {
        return 1;
    }

    is_running_ = true;
    while (is_running_)
    {
        auto event = input_pump_->pollEvent();
        if (event.has_value()) root_->processEvent(event);

        root_->render();
        renderer_->update();
    }

    return 0;
}

#endif
