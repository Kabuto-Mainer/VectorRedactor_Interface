#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDesktop.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/Package.hpp"
#include "Plugins/kabuto/ShapeFactory/export/IShapeFactory.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/Vec2F.hpp"
#include "export/IUIFramework.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <utility>


template<typename T>
T* queryInterface(const IID& iid)
{
    void* raw = nullptr;
    if (ApplicationCore::instance().queryInterface(iid, &raw) != ApplicationCore::Error::SUCCESS)
    {
        return nullptr;
    }

    return static_cast<T*>(raw);
}

namespace Kabuto
{

namespace MainMenu
{

std::unique_ptr<IUIFramework::ButtonPool> createMainMenu(IUIFramework& framework)
{
    IUIFramework::ButtonPoolDescription button_pool;
    button_pool.min_point = Vec2F(500, 0);
    button_pool.size      = Vec2F(400, 90);
    button_pool.direction = IUIFramework::Direction::HORIZONTAL;
    button_pool.role      = IUIFramework::Role::DEFAULT;

    std::unique_ptr<IUIFramework::ButtonPool> pool = framework.create(button_pool);
    if (!pool)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "MainMenu can't create a button pool");
        return nullptr;
    }

    return pool;
}

void add(IUIFramework& framework, IUIFramework::ButtonPool& pool, IShapeFactory& shape_factory)
{
    IUIFramework::ButtonDescription load;
    load.text = "LOAD";
    load.on_press.callbacks.emplace_back([&shape_factory] { shape_factory.loadFrom("example.json"); });
    load.box = BoxF(0, 0, 80, 30);

    std::unique_ptr<IUIFramework::Button> load_b = framework.create(load);
    if (!load_b)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "MainMenu can't create a button LOAD");
        return;
    }
    pool.add(std::move(load_b));

    IUIFramework::ButtonDescription save;
    save.text = "SAVE";
    save.on_press.callbacks.emplace_back([&shape_factory] { shape_factory.saveTo("example.json"); });
    save.box = BoxF(0, 0, 80, 30);

    std::unique_ptr<IUIFramework::Button> save_b = framework.create(save);
    if (!save_b)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "MainMenu can't create a button SAVE");
        return;
    }
    pool.add(std::move(save_b));
}

} // namespace MainMenu

} // namespace Kabuto



extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    Kabuto::IUIFramework* framework = queryInterface<Kabuto::IUIFramework>(Kabuto::IID_IUIFramework);
    IRenderer*            renderer  = queryInterface<IRenderer>(IID_RENDERER);
    IDesktop*             desktop   = queryInterface<IDesktop>(IID_DESKTOP);

    Kabuto::IShapeFactory* shape_factory = queryInterface<Kabuto::IShapeFactory>(Kabuto::IID_ISHAPE_FACTORY);

    if (framework == nullptr || renderer == nullptr || desktop == nullptr || shape_factory == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL,
                             "MainMenu can't find the framework, the desktop, the renderer or the shape factory");
        return false;
    }

    auto pool = Kabuto::MainMenu::createMainMenu(*framework);
    if (pool == nullptr)
    {
        return false;
    }

    Kabuto::MainMenu::add(*framework, *pool.get(), *shape_factory);

    if (desktop->add(std::move(pool)) != IDesktop::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "MainMenu can't be added to the desktop");
        return false;
    }

    return true;
}
