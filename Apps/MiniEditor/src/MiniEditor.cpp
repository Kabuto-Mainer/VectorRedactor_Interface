#include "Core/ApplicationCore.hpp"
#include "EllipseShape.hpp"
#include "Interfaces/IDesktop.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/IToolManager.hpp"
#include "Interfaces/IUIFramework.hpp"
#include "Interfaces/Package.hpp"
#include "RectangleShape.hpp"
#include "ShapeTool.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/Vec2F.hpp"

#include <memory>
#include <string>
#include <utility>

namespace
{

constexpr IUIFramework::Role ROLE_DEFAULT = IUIFramework::Role::DEFAULT;

constexpr IToolManager::ATool::ID RECTANGLE_TOOL_ID = 1;
constexpr IToolManager::ATool::ID ELLIPSE_TOOL_ID   = 2;

const Vec2F SHAPE_SIZE{120.0F, 80.0F};

// The control panel is the window itself: clicks there belong to UIFW widgets, not to tools.
const BoxF PANEL_BOX{16.0F, 16.0F, 176.0F, 188.0F};

constexpr float POOL_ITEM_HEIGHT = 36.0F;
constexpr float POOL_TOP         = 36.0F;
constexpr float POOL_WIDTH       = 128.0F;
constexpr float PANEL_MARGIN     = 16.0F;
constexpr float THEME_HEIGHT     = 32.0F;

constexpr IRenderer::Color RECTANGLE_COLOR{53, 132, 228, 255};
constexpr IRenderer::Color ELLIPSE_COLOR{255, 123, 107, 255};

IUIFramework::Action action(std::function<void()> fn)
{
    IUIFramework::Action result;
    result.callbacks.push_back(std::move(fn));
    return result;
}

std::unique_ptr<IUIFramework::WindowMenu> buildEditorWindow(IUIFramework& framework, IToolManager& tool_manager)
{
    IUIFramework::WindowMenuDescription description;

    description.box      = PANEL_BOX;
    description.label    = std::string("Mini editor");
    description.role     = ROLE_DEFAULT;
    description.on_close = action([] { ApplicationCore::instance().stop(); });

    std::unique_ptr<IUIFramework::WindowMenu> window = framework.create(description);
    if (!window)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "UI framework failed to create window");
        return nullptr;
    }

    IUIFramework::SwitcherPoolDescription tools;
    tools.min_point = Vec2F(PANEL_MARGIN, POOL_TOP);
    tools.size      = Vec2F(POOL_WIDTH, POOL_ITEM_HEIGHT * 2.0F);
    tools.direction = IUIFramework::Direction::VERTICAL;
    tools.role      = ROLE_DEFAULT;

    std::unique_ptr<IUIFramework::SwitcherPool> tool_pool = framework.create(tools);
    if (!tool_pool)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "UI framework failed to create tool pool");
        return nullptr;
    }

    struct ToolEntry
    {
        const char*             text;
        IToolManager::ATool::ID tool_id;
    };

    const ToolEntry entries[] = {
        {"Rectangle", RECTANGLE_TOOL_ID},
        {  "Ellipse",   ELLIPSE_TOOL_ID}
    };

    for (const ToolEntry& entry : entries)
    {
        IUIFramework::SwitcherDescription switcher;
        switcher.text      = std::string(entry.text);
        switcher.role      = ROLE_DEFAULT;
        switcher.on_active = action([&tool_manager, tool_id = entry.tool_id] { tool_manager.setActiveTool(tool_id); });
        tool_pool->add(framework.create(switcher));
    }

    IUIFramework::SwitcherDescription theme;
    theme.box         = BoxF(PANEL_MARGIN, POOL_TOP + POOL_ITEM_HEIGHT * 2.0F + PANEL_MARGIN, POOL_WIDTH, THEME_HEIGHT);
    theme.text        = std::string("Dark theme");
    theme.role        = ROLE_DEFAULT;
    theme.on_active   = action([&framework] { framework.setTheme("dark"); });
    theme.on_deactive = action([&framework] { framework.setTheme("default"); });

    window->add(std::move(tool_pool));
    window->add(framework.create(theme));

    return window;
}

} // namespace

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    void*            raw  = nullptr;
    ApplicationCore& core = ApplicationCore::instance();

    if (core.queryInterface(IID_IUIFramework, &raw) != ApplicationCore::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "UI framework is not loaded");
        return false;
    }
    IUIFramework* framework = static_cast<IUIFramework*>(raw);
    raw                     = nullptr;

    if (core.queryInterface(IID_DESKTOP, &raw) != ApplicationCore::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Desktop is not loaded");
        return false;
    }
    IDesktop* desktop = static_cast<IDesktop*>(raw);
    raw               = nullptr;

    if (core.queryInterface(IID_IDOCUMENT, &raw) != ApplicationCore::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Document is not loaded");
        return false;
    }
    IDocument* document = static_cast<IDocument*>(raw);
    raw                 = nullptr;

    if (core.queryInterface(IID_TOOL_MANAGER, &raw) != ApplicationCore::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Tool manager is not loaded");
        return false;
    }
    IToolManager* tool_manager = static_cast<IToolManager*>(raw);
    raw                        = nullptr;

    auto rectangle =
        std::make_unique<MiniEditor::ShapeTool<VDocument::RectangleShape>>(RECTANGLE_TOOL_ID, *document, SHAPE_SIZE,
                                                                           PANEL_BOX, RECTANGLE_COLOR);
    auto ellipse =
        std::make_unique<MiniEditor::ShapeTool<VDocument::EllipseShape>>(ELLIPSE_TOOL_ID, *document, SHAPE_SIZE,
                                                                         PANEL_BOX, ELLIPSE_COLOR);

    // Регистрация должна выполняться для каждого инструмента: || и && через short-circuit пропустят вызовы.
    if (tool_manager->addTool(std::move(rectangle)) != IToolManager::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Failed to register rectangle tool");
        return false;
    }
    if (tool_manager->addTool(std::move(ellipse)) != IToolManager::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Failed to register ellipse tool");
        return false;
    }
    if (tool_manager->setActiveTool(RECTANGLE_TOOL_ID) != IToolManager::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Failed to activate rectangle tool");
        return false;
    }

    std::unique_ptr<IUIFramework::WindowMenu> window = buildEditorWindow(*framework, *tool_manager);
    if (!window)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Failed to create editor window");
        return false;
    }


    desktop->add(std::move(window));

    return true;
}
