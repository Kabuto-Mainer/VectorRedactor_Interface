#include "MainPlugin.hpp"

#include "Core/ApplicationCore.hpp"
#include "IUIFW.hpp"
#include "Interfaces/Package.hpp"
#include "Utils/AObject.hpp"
#include "Utils/BoxF.hpp"

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <variant>

namespace
{

UI::IFramework* queryFramework()
{
    void* raw = nullptr;
    if (ApplicationCore::instance().queryInterface(IID_IUIFramework, &raw) != ApplicationCore::Error::SUCCESS)
    {
        return nullptr;
    }
    return static_cast<UI::IFramework*>(raw);
}

UI::Action callback(std::function<void()> fn)
{
    UI::Action action;
    action.callbacks.push_back(std::move(fn));
    return action;
}

std::unique_ptr<AObject> buildWindow(UI::IFramework& framework)
{
    auto window = std::make_unique<UI::WindowDescription>();

    window->box   = BoxF(100.0F, 100.0F, 520.0F, 420.0F);
    window->label = std::string("UI Framework demo");
    window->role  = UI::Role::PRIMARY;

    window->on_event = [](const IInputPump::Event& event)
    {
        if (std::holds_alternative<IInputPump::EventQuit>(event))
        {
            ApplicationCore::instance().stop();
            return true;
        }
        return false;
    };
    window->on_close = callback([] { ApplicationCore::instance().stop(); });

    auto hello      = std::make_unique<UI::ButtonDescription>();
    hello->box      = BoxF(130.0F, 160.0F, 290.0F, 200.0F);
    hello->text     = std::string("Hello");
    hello->on_press = callback([] { ApplicationCore::log(ApplicationCore::LogLevel::OK, "Hello pressed"); });
    window->objects.push_back(std::move(hello));

    auto dark         = std::make_unique<UI::SwitcherDescription>();
    dark->box         = BoxF(310.0F, 160.0F, 470.0F, 200.0F);
    dark->text        = std::string("Dark theme");
    dark->on_active   = callback([&framework] { framework.setTheme("dark"); });
    dark->on_deactive = callback([&framework] { framework.setTheme("default"); });
    window->objects.push_back(std::move(dark));

    auto quit      = std::make_unique<UI::ButtonDescription>();
    quit->box      = BoxF(130.0F, 240.0F, 290.0F, 280.0F);
    quit->text     = std::string("Quit");
    quit->role     = UI::Role::DANGER;
    quit->on_press = callback([] { ApplicationCore::instance().stop(); });
    window->objects.push_back(std::move(quit));

    return framework.create(*window);
}

} // namespace

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    UI::IFramework* framework = queryFramework();
    if (framework == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "UI framework is not loaded");
        return false;
    }

    ApplicationCore::instance().registerInterface(IID_MAIN_PLUGIN, reinterpret_cast<void*>(&IMainPlugin::instance()));

    std::unique_ptr<AObject> window = buildWindow(*framework);
    if (!window)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Failed to create UI window");
        return false;
    }

    ApplicationCore::instance().root()->addChild(std::move(window));

    return true;
}

IMainPlugin& IMainPlugin::instance()
{
    static IMainPlugin instance;
    return instance;
}
