#include "AddShapeTool.hpp"
#include "CameraTool.hpp"
#include "Core/ApplicationCore.hpp"
#include "DeleteShapeTool.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/IToolBar.hpp"
#include "Interfaces/IToolManager.hpp"
#include "Interfaces/Package.hpp"
#include "SetColorTool.hpp"
#include "ToolIds.hpp"
#include "Utils/Vec2F.hpp"
#include "export/EllipseShape.hpp"
#include "export/ICamera.hpp"
#include "export/IPalette.hpp"
#include "export/RectangleShape.hpp"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace
{

constexpr const char* RECTANGLE_ICON = "assets/rectangle.png";
constexpr const char* ELLIPSE_ICON   = "assets/ellipse.png";
constexpr const char* DELETE_ICON    = "assets/clean.png";
constexpr const char* COLOR_ICON     = "assets/fill.png";
constexpr const char* CAMERA_ICON    = "assets/camera.png";

struct ToolBarItem
{
    IToolManager::ATool::ID tool_id;
    const char*             icon;
}; // struct ToolBarItem

constexpr ToolBarItem TOOL_BAR_ITEMS[] = {
    {   Kabuto::Tools::ADD_SHAPE_TOOL_ID, RECTANGLE_ICON},
    {Kabuto::Tools::DELETE_SHAPE_TOOL_ID,    DELETE_ICON},
    {   Kabuto::Tools::SET_COLOR_TOOL_ID,     COLOR_ICON},
    {      Kabuto::Tools::CAMERA_TOOL_ID,    CAMERA_ICON},
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

class ToolRegistrationGuard final
{
public:
    ~ToolRegistrationGuard()
    {
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

    std::unique_ptr<Kabuto::Tools::AddShapeTool> add_shape = makeAddShapeTool(*document, *palette, *camera, *renderer);

    if (tool_manager->addTool(std::move(add_shape)) != IToolManager::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Tools can't register the add shape tool");
        return false;
    }
    registrationGuard().remember(Kabuto::Tools::ADD_SHAPE_TOOL_ID);

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

    auto camera_tool = std::make_unique<Kabuto::Tools::CameraTool>(*camera);
    if (tool_manager->addTool(std::move(camera_tool)) != IToolManager::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Tools can't register the camera tool");
        return false;
    }
    registrationGuard().remember(Kabuto::Tools::CAMERA_TOOL_ID);

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
