#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDesktop.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/Package.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/Vec2F.hpp"
#include "kabuto/ShapeFactory/export/IShapeFactory.hpp"
#include "kabuto/UIFW/export/IUIFramework.hpp"

#include <cmath>
#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <json/json.h>
#include <limits>
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

namespace
{

constexpr const char* MAIN_MENU_CONFIG_FILENAME = "config.json";

struct MainMenuConfig
{
    float left          = 500.0F;
    float top           = 0.0F;
    float width         = 400.0F;
    float height        = 90.0F;
    float button_width  = 80.0F;
    float button_height = 30.0F;
};

std::filesystem::path configFilePath()
{
    Dl_info info = {};

    if (dladdr(reinterpret_cast<const void*>(&configFilePath), &info) != 0 && info.dli_fname != nullptr)
    {
        return std::filesystem::path(info.dli_fname).parent_path() / MAIN_MENU_CONFIG_FILENAME;
    }

    std::error_code error;
    return std::filesystem::current_path(error) / MAIN_MENU_CONFIG_FILENAME;
}

void logInvalidConfigField(const std::filesystem::path& path, const char* field)
{
    ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "MainMenu config '%s': invalid '%s', using default",
                         path.string().c_str(), field);
}

float readFloat(const Json::Value& root, const char* field, float fallback, bool positive_only,
                const std::filesystem::path& path)
{
    const Json::Value& value = root[field];

    if (value.isNull())
    {
        return fallback;
    }

    if (!value.isNumeric())
    {
        logInvalidConfigField(path, field);
        return fallback;
    }

    constexpr double FLOAT_MAX = static_cast<double>(std::numeric_limits<float>::max());

    const double number = value.asDouble();
    if (!std::isfinite(number) || number > FLOAT_MAX || number < -FLOAT_MAX || (positive_only && number <= 0.0))
    {
        logInvalidConfigField(path, field);
        return fallback;
    }

    return static_cast<float>(number);
}

MainMenuConfig loadMainMenuConfig(const std::filesystem::path& path)
{
    MainMenuConfig config;

    std::ifstream file(path);
    if (!file.is_open())
    {
        ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "MainMenu config '%s' not found, using defaults",
                             path.string().c_str());
        return config;
    }

    Json::CharReaderBuilder builder;
    Json::Value             root;
    std::string             errors;

    if (!Json::parseFromStream(builder, file, &root, &errors) || !root.isObject())
    {
        ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "MainMenu config '%s' is invalid (%s), using defaults",
                             path.string().c_str(), errors.empty() ? "root is not an object" : errors.c_str());
        return config;
    }

    config.left          = readFloat(root, "left", config.left, false, path);
    config.top           = readFloat(root, "top", config.top, false, path);
    config.width         = readFloat(root, "width", config.width, true, path);
    config.height        = readFloat(root, "height", config.height, true, path);
    config.button_width  = readFloat(root, "button_width", config.button_width, true, path);
    config.button_height = readFloat(root, "button_height", config.button_height, true, path);

    return config;
}

std::unique_ptr<IUIFramework::ButtonPool> createMainMenu(IUIFramework& framework, const MainMenuConfig& config)
{
    IUIFramework::ButtonPoolDescription button_pool;
    button_pool.min_point = Vec2F(config.left, config.top);
    button_pool.size      = Vec2F(config.width, config.height);
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

void add(IUIFramework& framework, IUIFramework::ButtonPool& pool, IShapeFactory& shape_factory,
         const MainMenuConfig& config)
{
    IUIFramework::ButtonDescription load;
    load.text = "LOAD";
    load.on_press.callbacks.emplace_back([&shape_factory] { shape_factory.loadFrom("example.json"); });
    load.box = BoxF(0, 0, config.button_width, config.button_height);

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
    save.box = BoxF(0, 0, config.button_width, config.button_height);

    std::unique_ptr<IUIFramework::Button> save_b = framework.create(save);
    if (!save_b)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "MainMenu can't create a button SAVE");
        return;
    }
    pool.add(std::move(save_b));
}

} // namespace

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

    const Kabuto::MainMenu::MainMenuConfig config =
        Kabuto::MainMenu::loadMainMenuConfig(Kabuto::MainMenu::configFilePath());

    auto pool = Kabuto::MainMenu::createMainMenu(*framework, config);
    if (pool == nullptr)
    {
        return false;
    }

    Kabuto::MainMenu::add(*framework, *pool.get(), *shape_factory, config);

    if (desktop->add(std::move(pool)) != IDesktop::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "MainMenu can't be added to the desktop");
        return false;
    }

    return true;
}
