#include "ToolBar.hpp"

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IToolManager.hpp"
#include "ToolManager.hpp"

#include <memory>
#include <utility>
#include <variant>

namespace
{

class SelectToolCommand final : public ACommand
{
public:
    SelectToolCommand(IToolManager& manager, IToolManager::ATool::ID tool_id) : manager_(manager), tool_id_(tool_id)
    {
    }

    void execute() const override
    {
        manager_.setActiveTool(tool_id_);
    }

    bool isActive() const
    {
        return static_cast<const ToolManager&>(manager_).activeToolId() == tool_id_;
    }

private:
    IToolManager&           manager_;
    IToolManager::ATool::ID tool_id_;
};

class ToolButton final : public UI::IconButton
{
public:
    ToolButton(IToolManager::ATool::ID tool_id, IRenderer::TextureID texture_id,
               std::unique_ptr<SelectToolCommand> command) :
        UI::IconButton(Vec2F{}, Vec2F(UI::BUTTON_SIZE), *command, texture_id, IRenderer::Color::DARK_GRAY),
        tool_id_(tool_id), command_(std::move(command))
    {
    }

    bool processEvent(const IInputPump::Event& event) override
    {
        const auto* mouse = std::get_if<IInputPump::EventMouse>(&event);
        if (mouse == nullptr ||
            mouse->type() != IInputPump::EventMouse::Type::LEFT_BUTTON_PRESSING ||
            !globalBox().contains(mouse->cursorPos()))
        {
            return false;
        }
        command_->execute();
        return true;
    }

    void render() override
    {
        setState(command_->isActive() ? UI::ButtonState::PRESSED : UI::ButtonState::NO_PRESSED);
        UI::IconButton::render();
    }

    IToolManager::ATool::ID toolId() const
    {
        return tool_id_;
    }

private:
    IToolManager::ATool::ID            tool_id_;
    std::unique_ptr<SelectToolCommand> command_;
};

ToolButton* findToolButton(TToolBar& toolbar, IToolManager::ATool::ID tool_id)
{
    for (const auto& child : toolbar.pool().buttons())
    {
        auto* button = dynamic_cast<ToolButton*>(child.get());
        if (button != nullptr && button->toolId() == tool_id)
        {
            return button;
        }
    }
    return nullptr;
}

} // namespace

TToolBar::TToolBar(const Vec2F& pos, UI::ButtonPoolDirection direc) : pool_(Vec2F{}, direc)
{
    setBox(BoxF(pos, pos));
    pool_.setParent(this);
}

bool TToolBar::processEvent(const IInputPump::Event& event)
{
    return pool_.processEvent(event);
}

void TToolBar::render()
{
    pool_.render();
}

TObject::ID TToolBar::addButton(std::unique_ptr<UI::Button> button)
{
    if (button == nullptr)
    {
        return 0;
    }

    const TObject::ID button_id = button->id();
    pool_.add(std::move(button));
    setBox(BoxF(box().topLeft(), box().topLeft() + pool_.box().size()));
    return button_id;
}

bool TToolBar::removeButton(TObject::ID button_id)
{
    if (!pool_.remove(button_id))
    {
        return false;
    }

    setBox(BoxF(box().topLeft(), box().topLeft() + pool_.box().size()));
    return true;
}

CToolBar::CToolBar(TToolBar& tool_bar) : tool_bar_(tool_bar)
{
}

void CToolBar::addTool(IToolManager::ATool::ID tool_id, IRenderer::TextureID texture_id)
{
    if (findToolButton(tool_bar_, tool_id) != nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Tool %llu is already in the toolbar",
                             static_cast<unsigned long long>(tool_id));
        return;
    }

    IToolManager* manager = nullptr;
    if (ApplicationCore::instance().queryInterface(IID_TOOL_MANAGER, reinterpret_cast<void**>(&manager)) !=
            ApplicationCore::Error::SUCCESS ||
        manager == nullptr)
    {
        return;
    }

    auto command = std::make_unique<SelectToolCommand>(*manager, tool_id);
    auto button  = std::make_unique<ToolButton>(tool_id, texture_id, std::move(command));
    tool_bar_.addButton(std::move(button));
}

void CToolBar::removeTool(IToolManager::ATool::ID tool_id)
{
    ToolButton* button = findToolButton(tool_bar_, tool_id);
    if (button == nullptr)
    {
        return;
    }

    tool_bar_.removeButton(button->id());
}
