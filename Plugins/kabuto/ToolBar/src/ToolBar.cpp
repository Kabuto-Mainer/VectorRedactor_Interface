#include "ToolBar.hpp"

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDesktop.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/IToolManager.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/Vec2F.hpp"

#include <algorithm>
#include <cmath>
#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <json/json.h>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace Kabuto
{

namespace ToolBar
{

namespace
{

constexpr const char* TOOL_BAR_CONFIG_FILENAME = "config.json";

BoxF toolBarBox(const ToolBarConfig& config)
{
    return BoxF(config.left, config.top, config.left + config.width, config.top + config.height);
}

std::filesystem::path configFilePath()
{
    Dl_info info = {};

    if (dladdr(reinterpret_cast<const void*>(&configFilePath), &info) != 0 && info.dli_fname != nullptr)
    {
        return std::filesystem::path(info.dli_fname).parent_path() / TOOL_BAR_CONFIG_FILENAME;
    }

    std::error_code error;
    return std::filesystem::current_path(error) / TOOL_BAR_CONFIG_FILENAME;
}

void logInvalidConfigField(const std::filesystem::path& path, const char* field)
{
    ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "ToolBar config '%s': invalid '%s', using default",
                         path.string().c_str(), field);
}

float readFloat(const Json::Value& root, const char* field, float fallback, bool positive_only,
                const std::filesystem::path& path)
{
    const Json::Value& value = root[field];

    if (value.isNull())
    {
        return fallback;
    }

    if (!value.isNumeric())
    {
        logInvalidConfigField(path, field);
        return fallback;
    }

    constexpr double FLOAT_MAX = static_cast<double>(std::numeric_limits<float>::max());

    const double number = value.asDouble();
    if (!std::isfinite(number) || number > FLOAT_MAX || number < -FLOAT_MAX || (positive_only && number <= 0.0))
    {
        logInvalidConfigField(path, field);
        return fallback;
    }

    return static_cast<float>(number);
}

std::string readString(const Json::Value& root, const char* field, const std::string& fallback,
                       const std::filesystem::path& path)
{
    const Json::Value& value = root[field];

    if (value.isNull())
    {
        return fallback;
    }

    if (!value.isString() || value.asString().empty())
    {
        logInvalidConfigField(path, field);
        return fallback;
    }

    return value.asString();
}

ToolBarConfig loadToolBarConfig(const std::filesystem::path& path)
{
    ToolBarConfig config;

    std::ifstream file(path);
    if (!file.is_open())
    {
        ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "ToolBar config '%s' not found, using defaults",
                             path.string().c_str());
        return config;
    }

    Json::CharReaderBuilder builder;
    Json::Value             root;
    std::string             errors;

    if (!Json::parseFromStream(builder, file, &root, &errors) || !root.isObject())
    {
        ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "ToolBar config '%s' is invalid (%s), using defaults",
                             path.string().c_str(), errors.empty() ? "root is not an object" : errors.c_str());
        return config;
    }

    config.left          = readFloat(root, "left", config.left, false, path);
    config.top           = readFloat(root, "top", config.top, false, path);
    config.width         = readFloat(root, "width", config.width, true, path);
    config.height        = readFloat(root, "height", config.height, true, path);
    config.header_height = readFloat(root, "header_height", config.header_height, true, path);
    config.padding       = readFloat(root, "padding", config.padding, true, path);
    config.row_height    = readFloat(root, "row_height", config.row_height, true, path);
    config.icon_size     = readFloat(root, "icon_size", config.icon_size, true, path);
    config.label         = readString(root, "label", config.label, path);

    return config;
}

template<typename T>
T* queryInterface(const IID& iid)
{
    void* raw = nullptr;
    if (ApplicationCore::instance().queryInterface(iid, &raw) != ApplicationCore::Error::SUCCESS)
    {
        return nullptr;
    }

    return static_cast<T*>(raw);
}

} // namespace

/* TToolBar */

void TToolBar::addTool(IToolManager::ATool::ID tool_id, IRenderer::TextureID texture_id)
{
    const auto found = std::find_if(entries_.begin(), entries_.end(),
                                    [tool_id](const Entry& entry) { return entry.tool_id == tool_id; });

    if (found != entries_.end())
    {
        found->texture_id = texture_id;
    }
    else
    {
        entries_.push_back(Entry{tool_id, texture_id});
    }

    rebuild();
}

void TToolBar::removeTool(IToolManager::ATool::ID tool_id)
{
    const auto removed = std::remove_if(entries_.begin(), entries_.end(),
                                        [tool_id](const Entry& entry) { return entry.tool_id == tool_id; });
    if (removed == entries_.end())
    {
        ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "Tool %llu is not in the tool bar",
                             static_cast<unsigned long long>(tool_id));
        return;
    }

    entries_.erase(removed, entries_.end());
    rebuild();
}

bool TToolBar::processEvent(const IInputPump::Event& event)
{
    if (processEventChildren(event))
    {
        return true;
    }

    if (entries_.empty())
    {
        return false;
    }

    const auto* mouse = std::get_if<IInputPump::EventMouse>(&event);
    return mouse != nullptr &&
           mouse->type() == IInputPump::EventMouse::Type::LEFT_BUTTON_PRESSING &&
           toolBarBox(config_).contains(mouse->cursorPos());
}

void TToolBar::render()
{
    renderChildren();
}

Vec2F TToolBar::mapLocalToGlobal(const Vec2F& pos) const
{
    return parent() != nullptr ? parent()->mapLocalToGlobal(pos) : pos;
}

Vec2F TToolBar::mapGlobalToLocal(const Vec2F& pos) const
{
    return parent() != nullptr ? parent()->mapGlobalToLocal(pos) : pos;
}

void TToolBar::rebuild()
{
    children().clear();

    if (entries_.empty())
    {
        return;
    }

    IUIFramework* framework = queryInterface<IUIFramework>(IID_IUIFramework);
    if (framework == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Tool bar can't find the UI framework");
        return;
    }

    IToolManager* tool_manager = queryInterface<IToolManager>(IID_TOOL_MANAGER);
    if (tool_manager == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Tool bar can't find the tool manager");
        return;
    }

    const BoxF  box        = toolBarBox(config_);
    const float rows       = config_.row_height * static_cast<float>(entries_.size());
    const float free_space = box.size().y() - config_.header_height - 2.0F * config_.padding;
    const float pool_size  = std::min(rows, free_space);

    if (pool_size <= 0.0F)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Tool bar has no room for tools");
        return;
    }

    IUIFramework::WindowMenuDescription window_description;
    window_description.box   = box;
    window_description.label = config_.label;
    window_description.role  = IUIFramework::Role::DEFAULT;

    std::unique_ptr<IUIFramework::WindowMenu> window = framework->create(window_description);
    if (!window)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Tool bar can't create its window");
        return;
    }

    IUIFramework::SwitcherPoolDescription pool_description;
    pool_description.min_point = Vec2F(config_.padding, config_.header_height + config_.padding);
    pool_description.size      = Vec2F(box.size().x() - 2.0F * config_.padding, pool_size);
    pool_description.direction = IUIFramework::Direction::VERTICAL;
    pool_description.role      = IUIFramework::Role::DEFAULT;

    std::unique_ptr<IUIFramework::SwitcherPool> pool = framework->create(pool_description);
    if (!pool)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Tool bar can't create its switcher pool");
        return;
    }

    for (const Entry& entry : entries_)
    {
        IUIFramework::SwitcherDescription switcher_description;
        switcher_description.icon = IUIFramework::Icon{entry.texture_id};
        switcher_description.role = IUIFramework::Role::DEFAULT;
        switcher_description.box  = BoxF{0, 0, config_.icon_size, config_.icon_size};
        switcher_description.on_active.callbacks.emplace_back(
            [tool_manager, tool_id = entry.tool_id]
            {
                if (tool_manager->setActiveTool(tool_id) != IToolManager::Error::SUCCESS)
                {
                    ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "Tool bar can't activate tool %llu",
                                         static_cast<unsigned long long>(tool_id));
                }
            }
        );

        std::unique_ptr<IUIFramework::Switcher> switcher = framework->create(switcher_description);
        if (!switcher)
        {
            ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Tool bar can't create a tool button");
            continue;
        }

        pool->add(std::move(switcher));
    }

    window->add(std::move(pool));
    addChild(std::move(window));
}

/* CToolBar */

CToolBar& CToolBar::instance()
{
    static CToolBar toolbar;
    return toolbar;
}

TToolBar* CToolBar::node() const
{
    TObject* object = ApplicationCore::instance().findObject(toolbar_id_);
    if (object == nullptr || object != toolbar_node_)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Disconnect between raw toolbar node and toolbar id");
        return nullptr;
    }

    return static_cast<TToolBar*>(object);
}

void CToolBar::bindNode(TToolBar& toolbar)
{
    toolbar_node_ = &toolbar;
    toolbar_id_   = toolbar.id();
}

void CToolBar::unbindNode()
{
    toolbar_node_ = nullptr;
}

void CToolBar::addTool(IToolManager::ATool::ID tool_id, IRenderer::TextureID texture_id)
{
    if (TToolBar* toolbar = node(); toolbar != nullptr)
    {
        toolbar->addTool(tool_id, texture_id);
    }
}

void CToolBar::removeTool(IToolManager::ATool::ID tool_id)
{
    if (TToolBar* toolbar = node(); toolbar != nullptr)
    {
        toolbar->removeTool(tool_id);
    }
}

}; // namespace ToolBar

}; // namespace Kabuto

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    ApplicationCore& core = ApplicationCore::instance();

    Kabuto::IUIFramework* framework = nullptr;
    if (core.queryInterface(Kabuto::IID_IUIFramework, reinterpret_cast<void**>(&framework)) !=
            ApplicationCore::Error::SUCCESS ||
        framework == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Tool bar can't find the UI framework");
        return false;
    }

    IToolManager* tool_manager = nullptr;
    if (core.queryInterface(IID_TOOL_MANAGER, reinterpret_cast<void**>(&tool_manager)) !=
            ApplicationCore::Error::SUCCESS ||
        tool_manager == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Tool bar can't find the tool manager");
        return false;
    }

    IDesktop* desktop = nullptr;
    if (core.queryInterface(IID_DESKTOP, reinterpret_cast<void**>(&desktop)) != ApplicationCore::Error::SUCCESS ||
        desktop == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Tool bar can't find the desktop");
        return false;
    }

    const Kabuto::ToolBar::ToolBarConfig config = Kabuto::ToolBar::loadToolBarConfig(Kabuto::ToolBar::configFilePath());

    auto  toolbar_node = std::make_unique<Kabuto::ToolBar::TToolBar>(config);
    auto* node_ptr     = toolbar_node.get();

    Kabuto::ToolBar::CToolBar& toolbar = Kabuto::ToolBar::CToolBar::instance();
    toolbar.bindNode(*node_ptr);

    if (desktop->add(std::move(toolbar_node)) != IDesktop::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Tool bar can't be added to the desktop");
        toolbar.unbindNode();
        return false;
    }

    const ApplicationCore::Error error =
        core.registerInterface(IID_TOOL_BAR, static_cast<void*>(static_cast<IToolBar*>(&toolbar)));
    if (error != ApplicationCore::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Failed to register IToolBar: %s",
                             ApplicationCore::strError(error));

        desktop->remove(node_ptr->id());
        toolbar.unbindNode();
        return false;
    }

    return true;
}
