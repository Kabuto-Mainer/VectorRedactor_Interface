#pragma once

#include "Interfaces/IToolBar.hpp"
#include "Interfaces/IToolManager.hpp"
#include "Interfaces/Package.hpp"
#include "Plugins/kabuto/UIFW/export/IUIFramework.hpp"
#include "Utils/TObject.hpp"

#include <string>
#include <vector>

extern "C" InitPackageFunc INIT_PACKAGE_FUNC_NAME;

namespace Kabuto
{

namespace ToolBar
{

struct ToolBarConfig
{
    float       left          = 312.0F;
    float       top           = 24.0F;
    float       width         = 40.0F;
    float       height        = 256.0F;
    float       header_height = 28.0F;
    float       padding       = 8.0F;
    float       row_height    = 32.0F;
    float       icon_size     = 30.0F;
    std::string label         = "Tools";
};

class TToolBar final : public TObject
{
public:
    explicit TToolBar(const ToolBarConfig& config = {}) : config_(config)
    {
    }

    ~TToolBar() override = default;

    TToolBar(const TToolBar&)            = delete;
    TToolBar(TToolBar&&)                 = delete;
    TToolBar& operator=(const TToolBar&) = delete;
    TToolBar& operator=(TToolBar&&)      = delete;

    void addTool(IToolManager::ATool::ID tool_id, IRenderer::TextureID texture_id);
    void removeTool(IToolManager::ATool::ID tool_id);

    bool  processEvent(const IInputPump::Event& event) override;
    void  render() override;
    Vec2F mapLocalToGlobal(const Vec2F& pos) const override;
    Vec2F mapGlobalToLocal(const Vec2F& pos) const override;

private:
    struct Entry
    {
        IToolManager::ATool::ID tool_id;
        IRenderer::TextureID    texture_id;
    }; // struct Entry

    void rebuild();

    ToolBarConfig      config_;
    std::vector<Entry> entries_;
}; // class TToolBar

class CToolBar final : public IToolBar
{
public:
    friend class TToolBar;

    CToolBar(const CToolBar&)            = delete;
    CToolBar(CToolBar&&)                 = delete;
    CToolBar& operator=(const CToolBar&) = delete;
    CToolBar& operator=(CToolBar&&)      = delete;

    static CToolBar& instance();

    void bindNode(TToolBar& toolbar);
    void unbindNode();

    void addTool(IToolManager::ATool::ID tool_id, IRenderer::TextureID texture_id) override;
    void removeTool(IToolManager::ATool::ID tool_id) override;

private:
    CToolBar()           = default;
    ~CToolBar() override = default;

    TToolBar* node() const;

    TToolBar*   toolbar_node_ = nullptr;
    TObject::ID toolbar_id_   = 0;
}; // class CToolBar

}; // namespace ToolBar

}; // namespace Kabuto
