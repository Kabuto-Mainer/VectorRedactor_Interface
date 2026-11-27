#pragma once

#include "Interfaces/IToolBar.hpp"
#include "Interfaces/Package.hpp"
#include "UIFramework.hpp"

#include <memory>
#include <optional>
#include <unordered_map>

extern "C" InitPackageFunc INIT_PACKAGE_FUNC_NAME;

namespace Murugov
{

class TToolBar final : public TWidget
{
private:
    UI::ButtonPool pool_;

public:
    explicit TToolBar(const Vec2F& pos, UI::ButtonPoolDirection direc);

    bool processEvent(const IInputPump::Event& event) override;
    void render() override;

    TObject::ID addButton(std::unique_ptr<UI::Button> button);
    bool        removeButton(TObject::ID button_id);

    UI::ButtonPool& pool() { return pool_; }
};

class ToolBar final : public IToolBar
{
private:
    bool                                                     initialized_ = false;
    TToolBar*                                                tool_bar_    = nullptr;
    TObject::ID                                              toolbar_id_  = 0;
    std::optional<IToolManager::ATool::ID>                   active_tool_;
    std::unordered_map<IToolManager::ATool::ID, TObject::ID> tool_buttons_;

    ToolBar();
    ~ToolBar() override;

    struct Config
    {
        float anchor_x = 16.0F;
        float anchor_y = 40.0F;
    };

    static Config loadConfig(const std::string& filepath);

public:
    ToolBar(const ToolBar&)            = delete;
    ToolBar(ToolBar&&)                 = delete;
    ToolBar& operator=(const ToolBar&) = delete;
    ToolBar& operator=(ToolBar&&)      = delete;

    static ToolBar& instance();

    bool isValid() const;

    void setActiveToolId(IToolManager::ATool::ID tool_id) { active_tool_ = tool_id; }
    bool isToolActive(IToolManager::ATool::ID tool_id) const { return active_tool_ == tool_id; }

    /* from IToolBar */

    void addTool(IToolManager::ATool::ID tool_id, IRenderer::TextureID texture_id) override;
    void removeTool(IToolManager::ATool::ID tool_id) override;
};

} // namespace Murugov