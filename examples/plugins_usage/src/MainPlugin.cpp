#include "MainPlugin.hpp"

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/Package.hpp"
#include "Utils/Vec2F.hpp"

#include <cassert>
#include <memory>
#include <variant>

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    ApplicationCore::instance().registerInterface(IID_MAIN_PLUGIN, reinterpret_cast<void*>(&IMainPlugin::instance()));

    auto main_window = std::make_unique<MainWindow>();
    ApplicationCore::instance().root()->addChild(std::move(main_window));

    return true;
}

void MainWindow::render()
{
    IRenderer* renderer = nullptr;
    if (ApplicationCore::instance().queryInterface(IID_RENDERER, reinterpret_cast<void**>(&renderer)) !=
        ApplicationCore::Error::SUCCESS)
    {
        return;
    }

    renderer->renderRect(Color{.r = 0x00, .g = 0xFF, .b = 0x00, .a = 0xFF}, BoxF{100.0F, 100.0F, 200.0F, 200.0F}, 0.0F);
}

bool MainWindow::processEvent(const Event& event)
{
    if (std::holds_alternative<EventQuit>(event))
    {
        ApplicationCore::instance().stop();
        return true;
    }

    return true;
}

Vec2F MainWindow::mapGlobalToLocal(const Vec2F &pos) const
{
	return pos;
}

Vec2F MainWindow::mapLocalToGlobal(const Vec2F &pos) const
{
	return pos;
}

IMainPlugin& IMainPlugin::instance()
{
    static IMainPlugin instance;
    return instance;
}
