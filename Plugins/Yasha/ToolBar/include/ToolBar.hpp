#pragma once

#include "Interfaces/IToolBar.hpp"
#include "UIFramework/UIFramework.hpp"

#include <memory>

class TToolBar : public TWidget
{
public:
    explicit TToolBar(const Vec2F& pos, UI::ButtonPoolDirection direc);

    bool processEvent(const IInputPump::Event& event) override;
    void render() override;

    TObject::ID addButton(std::unique_ptr<UI::Button> button);
    bool        removeButton(TObject::ID button_id);

    // clang-format off
    UI::ButtonPool& pool() { return pool_; }
    // clang-format on

private:
    UI::ButtonPool pool_;
};

class CToolBar : public IToolBar
{
public:
    explicit CToolBar(TToolBar& tool_bar);

    void addTool(IToolManager::ATool::ID tool_id, IRenderer::TextureID texture_id) override;
    void removeTool(IToolManager::ATool::ID tool_id) override;

private:
    TToolBar& tool_bar_;
};
