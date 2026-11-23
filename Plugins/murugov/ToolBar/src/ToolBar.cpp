#include "ToolBar.hpp"

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDesktop.hpp"
#include "Interfaces/IToolManager.hpp"
#include "Utils/ACommand.hpp"

#include <utility>
#include <variant>

namespace Murugov
{

namespace
{

class ExitCommand final : public ACommand
{
public:
    void execute() const override
    {
        ApplicationCore::instance().stop();
    }
};

class SelectToolCommand final : public ACommand
{
public:
    SelectToolCommand(IToolManager& manager, IToolManager::ATool::ID tool_id)
        : manager_(manager), tool_id_(tool_id)
    {
    }

    void execute() const override
    {
        manager_.setActiveTool(tool_id_);
        ToolBar::instance().setActiveToolId(tool_id_);
    }

    bool isActive() const
    {
        return ToolBar::instance().isToolActive(tool_id_);
    }

private:
    IToolManager&           manager_;
    IToolManager::ATool::ID tool_id_;
};

class ToolButton final : public UI::IconButton
{
public:
    ToolButton(IRenderer::TextureID texture_id, std::unique_ptr<SelectToolCommand> command)
        : UI::IconButton(Vec2F{}, Vec2F(UI::BUTTON_SIZE), *command, texture_id, IRenderer::Color::DARK_GRAY),
          command_(std::move(command))
    {
    }

    bool processEvent(const IInputPump::Event& event) override
    {
        const auto* mouse = std::get_if<IInputPump::EventMouse>(&event);
        if (mouse == nullptr || mouse->type() != IInputPump::EventMouse::Type::LEFT_BUTTON_PRESSING ||
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

private:
    std::unique_ptr<SelectToolCommand> command_;
};

} // namespace

/* TToolBar */

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

/* ToolBar */

ToolBar::ToolBar()
{
    auto& core = ApplicationCore::instance();

    IDesktop* desktop = nullptr;
    if (core.queryInterface(IID_DESKTOP, reinterpret_cast<void**>(&desktop)) != ApplicationCore::Error::SUCCESS ||
        desktop == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "ToolBar requires Desktop");
        return;
    }

    static ExitCommand exit_command;

    auto toolbar = std::make_unique<TToolBar>(Vec2F{16.0F, 16.0F}, UI::ButtonPoolDirection::VERTICAL);
    toolbar->addButton(std::make_unique<UI::ButtonText>(Vec2F{}, Vec2F{UI::BUTTON_SIZE}, "Exit", exit_command, 0,
                                                        IRenderer::Color::DARK_GRAY));

    tool_bar_   = toolbar.get();
    toolbar_id_ = toolbar->id();

    if (desktop->add(std::move(toolbar)) != IDesktop::Error::SUCCESS)
    {
        tool_bar_   = nullptr;
        toolbar_id_ = 0;
        return;
    }

    initialized_ = true;
}

ToolBar::~ToolBar()
{
    if (!initialized_ || toolbar_id_ == 0)
    {
        return;
    }

    IDesktop* desktop = nullptr;
    if (ApplicationCore::instance().queryInterface(IID_DESKTOP, reinterpret_cast<void**>(&desktop)) ==
            ApplicationCore::Error::SUCCESS &&
        desktop != nullptr)
    {
        desktop->remove(toolbar_id_);
    }
}

ToolBar& ToolBar::instance()
{
    static ToolBar inst;
    return inst;
}

bool ToolBar::isValid() const
{
    return initialized_;
}

void ToolBar::addTool(IToolManager::ATool::ID tool_id, IRenderer::TextureID texture_id)
{
    if (!initialized_ || tool_bar_ == nullptr)
    {
        return;
    }

    if (tool_buttons_.contains(tool_id))
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
    auto button  = std::make_unique<ToolButton>(texture_id, std::move(command));
    const TObject::ID btn_id  = tool_bar_->addButton(std::move(button));
    tool_buttons_[tool_id]    = btn_id;
}

void ToolBar::removeTool(IToolManager::ATool::ID tool_id)
{
    if (!initialized_ || tool_bar_ == nullptr)
    {
        return;
    }

    auto it = tool_buttons_.find(tool_id);
    if (it == tool_buttons_.end())
    {
        return;
    }

    tool_bar_->removeButton(it->second);
    tool_buttons_.erase(it);

    if (active_tool_ == tool_id)
    {
        active_tool_.reset();
    }
}

} // namespace Murugov

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    auto& toolbar = Murugov::ToolBar::instance();
    if (!toolbar.isValid())
    {
        return false;
    }

    return ApplicationCore::instance().registerInterface(IID_TOOL_BAR, static_cast<IToolBar*>(&toolbar)) ==
           ApplicationCore::Error::SUCCESS;
}