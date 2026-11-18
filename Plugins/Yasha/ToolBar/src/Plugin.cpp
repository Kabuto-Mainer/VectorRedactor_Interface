#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDesktop.hpp"
#include "Interfaces/Package.hpp"
#include "ToolBar.hpp"
#include "ToolBarIID.hpp"
#include "Utils/ACommand.hpp"

#include <memory>

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

ExitCommand& exitCommand()
{
    static ExitCommand command;
    return command;
}

std::unique_ptr<CToolBar>& controller()
{
    static std::unique_ptr<CToolBar> instance;
    return instance;
}

} // namespace

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    auto& core     = ApplicationCore::instance();
    void* existing = nullptr;
    if (core.queryInterface(IID_TOOL_BAR, &existing) == ApplicationCore::Error::SUCCESS)
    {
        return false;
    }
    IDesktop* desktop = nullptr;
    if (core.queryInterface(IID_DESKTOP, reinterpret_cast<void**>(&desktop)) != ApplicationCore::Error::SUCCESS ||
        desktop == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "ToolBar requires Desktop");
        return false;
    }

    auto toolbar = std::make_unique<TToolBar>(Vec2F{16.0F, 16.0F}, UI::ButtonPoolDirection::VERTICAL);
    toolbar->addButton(std::make_unique<UI::ButtonText>(Vec2F{}, Vec2F{UI::BUTTON_SIZE}, "Exit", exitCommand(), 0,
                                                        IRenderer::Color::DARK_GRAY));
    auto       next_controller = std::make_unique<CToolBar>(*toolbar);
    const auto toolbar_id      = toolbar->id();
    if (desktop->add(std::move(toolbar)) != IDesktop::Error::SUCCESS)
    {
        return false;
    }
    if (core.registerInterface(IID_TOOL_BAR, static_cast<IToolBar*>(next_controller.get())) !=
        ApplicationCore::Error::SUCCESS)
    {
        desktop->remove(toolbar_id);
        return false;
    }

    controller() = std::move(next_controller);
    return true;
}
