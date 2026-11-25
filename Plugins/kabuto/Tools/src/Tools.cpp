#include "AddPolygonTool.hpp"
#include "AddShapeTool.hpp"
#include "CameraTool.hpp"
#include "Core/ApplicationCore.hpp"
#include "DeleteShapeTool.hpp"
#include "EditPolygonTool.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/IToolBar.hpp"
#include "Interfaces/IToolManager.hpp"
#include "Interfaces/Package.hpp"
#include "Plugins/kabuto/ToolManager/export/IToolManagerPro.hpp"
#include "SetColorTool.hpp"
#include "ToolIds.hpp"
#include "Utils/Vec2F.hpp"
#include "export/EllipseShape.hpp"
#include "export/ICamera.hpp"
#include "export/IPalette.hpp"
#include "export/RectangleShape.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{

constexpr const char* RECTANGLE_ICON    = "assets/rectangle.png";
constexpr const char* ELLIPSE_ICON      = "assets/ellipse.png";
constexpr const char* POLYGON_ICON      = "assets/polygon.png";
constexpr const char* EDIT_POLYGON_ICON = "assets/edit_polygon.png";
constexpr const char* DELETE_ICON       = "assets/clean.png";
constexpr const char* COLOR_ICON        = "assets/fill.png";
constexpr const char* CAMERA_ICON       = "assets/camera.png";

struct ToolBarItem
{
    IToolManager::ATool::ID tool_id;
    const char*             icon;
}; // struct ToolBarItem

constexpr ToolBarItem TOOL_BAR_ITEMS[] = {
    {   Kabuto::Tools::ADD_SHAPE_TOOL_ID,    RECTANGLE_ICON},
    { Kabuto::Tools::ADD_POLYGON_TOOL_ID,      POLYGON_ICON},
    {Kabuto::Tools::EDIT_POLYGON_TOOL_ID, EDIT_POLYGON_ICON},
    {Kabuto::Tools::DELETE_SHAPE_TOOL_ID,       DELETE_ICON},
    {   Kabuto::Tools::SET_COLOR_TOOL_ID,        COLOR_ICON},
    {      Kabuto::Tools::CAMERA_TOOL_ID,       CAMERA_ICON},
};

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

/**
 * Keeps a deactivation callback for every tool that owns auxiliary objects.
 * The tool manager only knows the common ATool interface, so the plugin itself
 * maps tool ids to the concrete tools it has created.
 */
class ToolDeactivationRegistry final
{
public:
    using Callback = std::function<void()>;

    void remember(IToolManager::ATool::ID tool_id, Callback callback)
    {
        callbacks_.insert_or_assign(tool_id, std::move(callback));
    }

    void forget(IToolManager::ATool::ID tool_id)
    {
        callbacks_.erase(tool_id);
    }

    void deactivate(IToolManager::ATool::ID tool_id) const
    {
        const auto found = callbacks_.find(tool_id);
        if (found != callbacks_.end())
        {
            found->second();
        }
    }

    void deactivateAll() const
    {
        for (const auto& [tool_id, callback] : callbacks_)
        {
            callback();
        }
    }

    void clear()
    {
        callbacks_.clear();
    }

private:
    std::unordered_map<IToolManager::ATool::ID, Callback> callbacks_;
}; // class ToolDeactivationRegistry

ToolDeactivationRegistry& deactivationRegistry()
{
    static ToolDeactivationRegistry registry;
    return registry;
}

class ToolRegistrationGuard final
{
public:
    ~ToolRegistrationGuard()
    {
        deactivationRegistry().deactivateAll();
        deactivationRegistry().clear();

        IToolManager* manager = queryInterface<IToolManager>(IID_TOOL_MANAGER);
        if (manager == nullptr)
        {
            return;
        }

        for (const IToolManager::ATool::ID tool_id : ids_)
        {
            manager->removeTool(tool_id);
        }
    }

    void remember(IToolManager::ATool::ID tool_id)
    {
        ids_.push_back(tool_id);
    }

private:
    std::vector<IToolManager::ATool::ID> ids_;
}; // class ToolRegistrationGuard

ToolRegistrationGuard& registrationGuard()
{
    static ToolRegistrationGuard guard;
    return guard;
}

void subscribeToToolSwitches()
{
    Kabuto::IToolManagerPro* manager = queryInterface<Kabuto::IToolManagerPro>(Kabuto::IID_TOOL_MANAGER_PRO);
    if (manager == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::WARNING,
                             "Tools can't observe tool switches: IToolManagerPro is not available");
        return;
    }

    manager->followSwitchTool(
        [](Kabuto::IToolManagerPro::ID previous, Kabuto::IToolManagerPro::ID current)
        {
            /* NO_TOOL means the previous tool is being removed and has already
               been destroyed, so only the registration guard may clean it up. */
            if (current == Kabuto::IToolManagerPro::NO_TOOL)
            {
                return;
            }

            deactivationRegistry().deactivate(previous);
        }
    );

    manager->followRemoveTool([](Kabuto::IToolManagerPro::ID tool_id) { deactivationRegistry().forget(tool_id); });
}

std::optional<IRenderer::TextureID> loadIcon(IRenderer& renderer, const char* path)
{
    std::optional<IRenderer::TextureID> texture = renderer.loadTexture(path);
    if (!texture)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "Tools can't load the icon '%s'", path);
    }

    return texture;
}

Kabuto::Tools::AddShapeTool::Factory rectangleFactory()
{
    return [](const Vec2F& size) -> std::unique_ptr<IDocument::TShape>
    { return std::make_unique<Kabuto::Shapes::RectangleShape>(size); };
}

Kabuto::Tools::AddShapeTool::Factory ellipseFactory()
{
    return [](const Vec2F& size) -> std::unique_ptr<IDocument::TShape>
    { return std::make_unique<Kabuto::Shapes::EllipseShape>(size); };
}

std::unique_ptr<Kabuto::Tools::AddShapeTool> makeAddShapeTool(IDocument& document, Kabuto::IPalette& palette,
                                                              Kabuto::ICamera& camera, IRenderer& renderer)
{
    const std::optional<IRenderer::TextureID> rectangle_icon = loadIcon(renderer, RECTANGLE_ICON);
    const std::optional<IRenderer::TextureID> ellipse_icon   = loadIcon(renderer, ELLIPSE_ICON);

    std::vector<Kabuto::Tools::AddShapeTool::ShapeOption> options;
    options.push_back(Kabuto::Tools::AddShapeTool::ShapeOption{"Rectangle", rectangle_icon, rectangleFactory()});
    options.push_back(Kabuto::Tools::AddShapeTool::ShapeOption{"Ellipse", ellipse_icon, ellipseFactory()});

    return std::make_unique<Kabuto::Tools::AddShapeTool>(document, palette, camera, std::move(options));
}

} // namespace

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    IDocument*        document     = queryInterface<IDocument>(IID_IDOCUMENT);
    IToolManager*     tool_manager = queryInterface<IToolManager>(IID_TOOL_MANAGER);
    Kabuto::IPalette* palette      = queryInterface<Kabuto::IPalette>(Kabuto::IID_PALETTE);
    Kabuto::ICamera*  camera       = queryInterface<Kabuto::ICamera>(Kabuto::IID_ICAMERA);
    IToolBar*         tool_bar     = queryInterface<IToolBar>(IID_TOOL_BAR);
    IRenderer*        renderer     = queryInterface<IRenderer>(IID_RENDERER);

    if (document == nullptr ||
        tool_manager == nullptr ||
        palette == nullptr ||
        camera == nullptr ||
        tool_bar == nullptr ||
        renderer == nullptr)
    {
        ApplicationCore::log(
            ApplicationCore::LogLevel::CRITICAL,
            "Tools can't find the document, the tool manager, the palette, the camera, the tool bar or the renderer"
        );
        return false;
    }

    /* The registration guard deactivates the tools on unload, so its registry
       has to be constructed first and therefore destroyed last. */
    (void)deactivationRegistry();
    subscribeToToolSwitches();

    std::unique_ptr<Kabuto::Tools::AddShapeTool> add_shape = makeAddShapeTool(*document, *palette, *camera, *renderer);
    Kabuto::Tools::AddShapeTool*                 add_shape_tool = add_shape.get();

    if (tool_manager->addTool(std::move(add_shape)) != IToolManager::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Tools can't register the add shape tool");
        return false;
    }
    registrationGuard().remember(Kabuto::Tools::ADD_SHAPE_TOOL_ID);
    deactivationRegistry().remember(Kabuto::Tools::ADD_SHAPE_TOOL_ID,
                                    [add_shape_tool] { add_shape_tool->deactivate(); });

    auto add_polygon = std::make_unique<Kabuto::Tools::AddPolygonTool>(*document, *palette, *camera);
    Kabuto::Tools::AddPolygonTool* add_polygon_tool = add_polygon.get();
    if (tool_manager->addTool(std::move(add_polygon)) != IToolManager::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Tools can't register the add polygon tool");
        return false;
    }
    registrationGuard().remember(Kabuto::Tools::ADD_POLYGON_TOOL_ID);
    deactivationRegistry().remember(Kabuto::Tools::ADD_POLYGON_TOOL_ID,
                                    [add_polygon_tool] { add_polygon_tool->deactivate(); });

    auto                            edit_polygon      = std::make_unique<Kabuto::Tools::EditPolygonTool>(*document);
    Kabuto::Tools::EditPolygonTool* edit_polygon_tool = edit_polygon.get();
    if (tool_manager->addTool(std::move(edit_polygon)) != IToolManager::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Tools can't register the edit polygon tool");
        return false;
    }
    registrationGuard().remember(Kabuto::Tools::EDIT_POLYGON_TOOL_ID);
    deactivationRegistry().remember(Kabuto::Tools::EDIT_POLYGON_TOOL_ID,
                                    [edit_polygon_tool] { edit_polygon_tool->deactivate(); });

    auto delete_shape = std::make_unique<Kabuto::Tools::DeleteShapeTool>(*document);
    if (tool_manager->addTool(std::move(delete_shape)) != IToolManager::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Tools can't register the delete shape tool");
        return false;
    }
    registrationGuard().remember(Kabuto::Tools::DELETE_SHAPE_TOOL_ID);

    auto set_color = std::make_unique<Kabuto::Tools::SetColorTool>(*document, *palette);
    if (tool_manager->addTool(std::move(set_color)) != IToolManager::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Tools can't register the set color tool");
        return false;
    }
    registrationGuard().remember(Kabuto::Tools::SET_COLOR_TOOL_ID);

    auto                       camera_tool     = std::make_unique<Kabuto::Tools::CameraTool>(*camera);
    Kabuto::Tools::CameraTool* camera_tool_raw = camera_tool.get();
    if (tool_manager->addTool(std::move(camera_tool)) != IToolManager::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Tools can't register the camera tool");
        return false;
    }
    registrationGuard().remember(Kabuto::Tools::CAMERA_TOOL_ID);
    deactivationRegistry().remember(Kabuto::Tools::CAMERA_TOOL_ID,
                                    [camera_tool_raw] { camera_tool_raw->deactivate(); });

    for (const ToolBarItem& item : TOOL_BAR_ITEMS)
    {
        const std::optional<IRenderer::TextureID> icon = loadIcon(*renderer, item.icon);
        if (!icon)
        {
            continue;
        }

        tool_bar->addTool(item.tool_id, *icon);
    }

    if (tool_manager->setActiveTool(Kabuto::Tools::ADD_SHAPE_TOOL_ID) != IToolManager::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Tools can't activate the add shape tool");
        return false;
    }

    return true;
}
