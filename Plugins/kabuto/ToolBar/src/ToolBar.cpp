#include "ToolBar.hpp"

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDesktop.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/IToolManager.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/Vec2F.hpp"

#include <algorithm>
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

constexpr float TOOL_BAR_LEFT   = 312.0F;
constexpr float TOOL_BAR_TOP    = 24.0F;
constexpr float TOOL_BAR_RIGHT  = 352.0F;
constexpr float TOOL_BAR_BOTTOM = 228.0F;

constexpr float HEADER_HEIGHT = 28.0F;
constexpr float PADDING       = 8.0F;
constexpr float PADDING_SUM   = 2.0F * PADDING;
constexpr float ROW_HEIGHT    = 32.0F;

BoxF toolBarBox()
{
    return BoxF(TOOL_BAR_LEFT, TOOL_BAR_TOP, TOOL_BAR_RIGHT, TOOL_BAR_BOTTOM);
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
           toolBarBox().contains(mouse->cursorPos());
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

    const BoxF  box        = toolBarBox();
    const float rows       = ROW_HEIGHT * static_cast<float>(entries_.size());
    const float free_space = box.size().y() - HEADER_HEIGHT - PADDING_SUM;
    const float pool_size  = std::min(rows, free_space);

    if (pool_size <= 0.0F)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Tool bar has no room for tools");
        return;
    }

    IUIFramework::WindowMenuDescription window_description;
    window_description.box   = box;
    window_description.label = std::string("Tools");
    window_description.role  = IUIFramework::Role::DEFAULT;

    std::unique_ptr<IUIFramework::WindowMenu> window = framework->create(window_description);
    if (!window)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Tool bar can't create its window");
        return;
    }

    IUIFramework::SwitcherPoolDescription pool_description;
    pool_description.min_point = Vec2F(PADDING, HEADER_HEIGHT + PADDING);
    pool_description.size      = Vec2F(box.size().x() - PADDING_SUM, pool_size);
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

    IUIFramework* framework = nullptr;
    if (core.queryInterface(IID_IUIFramework, reinterpret_cast<void**>(&framework)) !=
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

    auto  toolbar_node = std::make_unique<Kabuto::ToolBar::TToolBar>();
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
