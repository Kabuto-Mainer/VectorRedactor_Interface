#pragma once

#include "IRenderer.hpp"
#include "IToolManager.hpp"
#include "Interfaces/IID.hpp"

constexpr static IID IID_TOOLBAR = 2;

class IToolBar
{
public:
    IToolBar()          = default;
    virtual ~IToolBar() = default;

    IToolBar(const IToolBar&)            = delete;
    IToolBar(IToolBar&&)                 = delete;
    IToolBar& operator=(const IToolBar&) = delete;
    IToolBar& operator=(IToolBar&&)      = delete;

    virtual void addTool(IToolManager::ATool::ID tool_id, IRenderer::TextureID texture_id) = 0;
    virtual void removeTool(IToolManager::ATool::ID tool_id)                               = 0;
}; // IToolBar
