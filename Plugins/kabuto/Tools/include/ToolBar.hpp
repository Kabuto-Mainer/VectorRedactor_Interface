#pragma once

#include "IUIFramework.hpp"
#include "Interfaces/IToolManager.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/Vec2F.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace Kabuto
{

namespace Tools
{

struct ToolBarEntry
{
    IToolManager::ATool::ID id;
    const char*             label;
}; // struct ToolBarEntry

inline BoxF toolBarBox()
{
    return BoxF(312.0F, 24.0F, 492.0F, 228.0F);
}

inline std::unique_ptr<IUIFramework::WindowMenu> buildToolBar(IUIFramework& framework, IToolManager& tool_manager,
                                                              const std::vector<ToolBarEntry>& entries)
{
    constexpr float HEADER_HEIGHT = 28.0F;
    constexpr float PADDING       = 8.0F;
    constexpr float ROW_HEIGHT    = 32.0F;

    const BoxF  box         = toolBarBox();
    const float rows_height = ROW_HEIGHT * static_cast<float>(entries.size());

    IUIFramework::WindowMenuDescription description;
    description.box   = box;
    description.label = std::string("Tools");
    description.role  = IUIFramework::Role::DEFAULT;

    std::unique_ptr<IUIFramework::WindowMenu> window = framework.create(description);
    if (!window)
    {
        return nullptr;
    }

    IUIFramework::SwitcherPoolDescription pool_description;
    pool_description.min_point = Vec2F(PADDING, HEADER_HEIGHT + PADDING);
    pool_description.size      = Vec2F(box.size().x() - 2.0F * PADDING, rows_height);
    pool_description.direction = IUIFramework::Direction::VERTICAL;
    pool_description.role      = IUIFramework::Role::DEFAULT;

    std::unique_ptr<IUIFramework::SwitcherPool> pool = framework.create(pool_description);
    if (!pool)
    {
        return nullptr;
    }

    for (const ToolBarEntry& entry : entries)
    {
        IUIFramework::SwitcherDescription switcher;
        switcher.text = std::string(entry.label);
        switcher.role = IUIFramework::Role::DEFAULT;
        switcher.on_active.callbacks.emplace_back([&tool_manager, id = entry.id] { tool_manager.setActiveTool(id); });

        std::unique_ptr<IUIFramework::Switcher> widget = framework.create(switcher);
        if (!widget)
        {
            return nullptr;
        }

        pool->add(std::move(widget));
    }

    window->add(std::move(pool));
    return window;
}

}; // namespace Tools

}; // namespace Kabuto
