#include "AddShapeTool.hpp"
#include "Core/ApplicationCore.hpp"
#include "DeleteShapeTool.hpp"
#include "EllipseShape.hpp"
#include "IPalette.hpp"
#include "Interfaces/IDesktop.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/IToolManager.hpp"
#include "Interfaces/Package.hpp"
#include "RectangleShape.hpp"
#include "ToolBar.hpp"
#include "ToolDispatcher.hpp"
#include "ToolIds.hpp"
#include "Utils/TObject.hpp"
#include "Utils/Vec2F.hpp"

#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

namespace
{

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

void moveAfter(TObject& parent, TObject* reference, TObject* child)
{
    if (reference == nullptr || child == nullptr)
    {
        return;
    }

    auto& children = parent.children();

    const auto child_position =
        std::find_if(children.begin(), children.end(),
                     [child](const std::unique_ptr<TObject>& candidate) { return candidate.get() == child; });
    const auto reference_position =
        std::find_if(children.begin(), children.end(),
                     [reference](const std::unique_ptr<TObject>& candidate) { return candidate.get() == reference; });

    if (child_position == children.end() ||
        reference_position == children.end() ||
        child_position == reference_position)
    {
        return;
    }

    auto after_reference = std::next(reference_position);
    if (child_position == after_reference)
    {
        return;
    }

    children.splice(after_reference, children, child_position);
}

/**
 * Tools are owned by ToolManager, which is loaded before this package and
 * therefore unloaded after it. Removing the tools here makes sure their
 * destructors run while the code that implements them is still loaded.
 */
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

        for (const IToolManager::ATool::ID id : ids_)
        {
            manager->removeTool(id);
        }
    }

    void remember(IToolManager::ATool::ID id)
    {
        ids_.push_back(id);
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

} // namespace

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    IDocument*    document     = queryInterface<IDocument>(IID_IDOCUMENT);
    IToolManager* tool_manager = queryInterface<IToolManager>(IID_TOOL_MANAGER);
    IPalette*     palette      = queryInterface<IPalette>(IID_PALETTE);
    IUIFramework* framework    = queryInterface<IUIFramework>(IID_IUIFramework);
    IDesktop*     desktop      = queryInterface<IDesktop>(IID_DESKTOP);

    if (document == nullptr || tool_manager == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Tools can't find the document or the tool manager");
        return false;
    }

    if (palette == nullptr || framework == nullptr || desktop == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL,
                             "Tools can't find the palette, the UI framework or the desktop");
        return false;
    }

    const BoxF ui_box = Kabuto::Tools::toolBarBox();

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

    if (tool_manager->setActiveTool(Kabuto::Tools::ADD_RECTANGLE_TOOL_ID) != IToolManager::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Tools can't activate the rectangle tool");
        return false;
    }

    auto  dispatcher     = std::make_unique<Kabuto::Tools::ToolDispatcher>(*tool_manager);
    auto* dispatcher_raw = dispatcher.get();

    if (desktop->add(std::move(dispatcher)) != IDesktop::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Tools can't add the dispatcher to the desktop");
        return false;
    }

    if (auto* document_node = dynamic_cast<TObject*>(document);
        document_node != nullptr && document_node->parent() != nullptr)
    {
        moveAfter(*document_node->parent(), document_node, dispatcher_raw);
    }

    const std::vector<Kabuto::Tools::ToolBarEntry> entries = {
        {Kabuto::Tools::ADD_RECTANGLE_TOOL_ID, "Add rectangle"},
        {  Kabuto::Tools::ADD_ELLIPSE_TOOL_ID,   "Add ellipse"},
        {       Kabuto::Tools::DELETE_TOOL_ID,        "Delete"},
    };

    std::unique_ptr<IUIFramework::WindowMenu> tool_bar =
        Kabuto::Tools::buildToolBar(*framework, *tool_manager, entries);
    if (!tool_bar)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Tools can't build the tool bar");
        return false;
    }

    if (desktop->add(std::move(tool_bar)) != IDesktop::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Tools can't add the tool bar to the desktop");
        return false;
    }

    return true;
}
