#include "AddShapeTool.hpp"
#include "Core/ApplicationCore.hpp"
#include "DeleteShapeTool.hpp"
#include "EllipseShape.hpp"
#include "IPalette.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/IToolBar.hpp"
#include "Interfaces/IToolManager.hpp"
#include "Interfaces/Package.hpp"
#include "RectangleShape.hpp"
#include "ToolIds.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/Vec2F.hpp"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace
{

constexpr float TOOL_BAR_LEFT   = 312.0F;
constexpr float TOOL_BAR_TOP    = 24.0F;
constexpr float TOOL_BAR_RIGHT  = 492.0F;
constexpr float TOOL_BAR_BOTTOM = 228.0F;

struct ToolBarItem
{
    IToolManager::ATool::ID tool_id;
    const char*             icon;
}; // struct ToolBarItem

constexpr ToolBarItem TOOL_BAR_ITEMS[] = {
    {Kabuto::Tools::ADD_RECTANGLE_TOOL_ID, "assets/rectangle.png"},
    {  Kabuto::Tools::ADD_ELLIPSE_TOOL_ID,   "assets/ellipse.png"},
    {       Kabuto::Tools::DELETE_TOOL_ID,       "assets/pen.png"},
};

BoxF toolBarBox()
{
    return BoxF(TOOL_BAR_LEFT, TOOL_BAR_TOP, TOOL_BAR_RIGHT, TOOL_BAR_BOTTOM);
}

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
};

ToolRegistrationGuard& registrationGuard()
{
    static ToolRegistrationGuard guard;
    return guard;
}

std::unique_ptr<Kabuto::Tools::AddShapeTool> makeRectangleTool(IDocument& document, IPalette& palette,
                                                               const BoxF& ui_box)
{
    return std::make_unique<Kabuto::Tools::AddShapeTool>(
        Kabuto::Tools::ADD_RECTANGLE_TOOL_ID, document, palette, ui_box, [](const Vec2F& size)
        { return std::unique_ptr<IDocument::TShape>(std::make_unique<Kabuto::Document::RectangleShape>(size)); }
    );
}

std::unique_ptr<Kabuto::Tools::AddShapeTool> makeEllipseTool(IDocument& document, IPalette& palette, const BoxF& ui_box)
{
    return std::make_unique<Kabuto::Tools::AddShapeTool>(
        Kabuto::Tools::ADD_ELLIPSE_TOOL_ID, document, palette, ui_box, [](const Vec2F& size)
        { return std::unique_ptr<IDocument::TShape>(std::make_unique<Kabuto::Document::EllipseShape>(size)); }
    );
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

} // namespace

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    IDocument*    document     = queryInterface<IDocument>(IID_IDOCUMENT);
    IToolManager* tool_manager = queryInterface<IToolManager>(IID_TOOL_MANAGER);
    IPalette*     palette      = queryInterface<IPalette>(IID_PALETTE);
    IToolBar*     tool_bar     = queryInterface<IToolBar>(IID_TOOL_BAR);
    IRenderer*    renderer     = queryInterface<IRenderer>(IID_RENDERER);

    if (document == nullptr || tool_manager == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Tools can't find the document or the tool manager");
        return false;
    }

    if (palette == nullptr || tool_bar == nullptr || renderer == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL,
                             "Tools can't find the palette, the tool bar or the renderer");
        return false;
    }

    const BoxF ui_box = toolBarBox();

    std::unique_ptr<Kabuto::Tools::AddShapeTool> rectangle = makeRectangleTool(*document, *palette, ui_box);
    std::unique_ptr<Kabuto::Tools::AddShapeTool> ellipse   = makeEllipseTool(*document, *palette, ui_box);

    if (tool_manager->addTool(std::move(rectangle)) != IToolManager::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Tools can't register the rectangle tool");
        return false;
    }
    registrationGuard().remember(Kabuto::Tools::ADD_RECTANGLE_TOOL_ID);

    if (tool_manager->addTool(std::move(ellipse)) != IToolManager::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Tools can't register the ellipse tool");
        return false;
    }
    registrationGuard().remember(Kabuto::Tools::ADD_ELLIPSE_TOOL_ID);

    auto delete_tool =
        std::make_unique<Kabuto::Tools::DeleteShapeTool>(Kabuto::Tools::DELETE_TOOL_ID, *document, ui_box);
    if (tool_manager->addTool(std::move(delete_tool)) != IToolManager::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Tools can't register the delete tool");
        return false;
    }
    registrationGuard().remember(Kabuto::Tools::DELETE_TOOL_ID);

    for (const ToolBarItem& item : TOOL_BAR_ITEMS)
    {
        const std::optional<IRenderer::TextureID> icon = loadIcon(*renderer, item.icon);
        if (!icon)
        {
            continue;
        }

        tool_bar->addTool(item.tool_id, *icon);
    }

    if (tool_manager->setActiveTool(Kabuto::Tools::ADD_RECTANGLE_TOOL_ID) != IToolManager::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Tools can't activate the rectangle tool");
        return false;
    }

    return true;
}
