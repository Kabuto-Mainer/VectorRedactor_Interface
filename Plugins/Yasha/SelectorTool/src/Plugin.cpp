#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDesktop.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/IToolBar.hpp"
#include "Interfaces/IToolManager.hpp"
#include "Interfaces/Package.hpp"
#include "QueryInterface.hpp"
#include "SelectorTool.hpp"
#include "ToolBarIID.hpp"
#include "ToolManager.hpp"

#include <memory>

namespace
{

class ToolRegistration final
{
public:
    ~ToolRegistration()
    {
        if (manager_ != nullptr)
        {
            manager_->removeTool(Yasha::Tools::SELECTOR_TOOL_ID);
        }
    }

    void setManager(IToolManager* manager)
    {
        manager_ = manager;
    }

    Yasha::Tools::SelectionState& state() { return state_; }

private:
    Yasha::Tools::SelectionState state_;
    IToolManager* manager_ = nullptr;
};

ToolRegistration& registration()
{
    static ToolRegistration guard;
    return guard;
}

} // namespace

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    auto* document = queryInterface<IDocument>(IID_IDOCUMENT);
    auto* desktop  = queryInterface<IDesktop>(IID_DESKTOP);
    auto* manager  = queryInterface<IToolManager>(IID_TOOL_MANAGER);
    auto* toolbar  = queryInterface<IToolBar>(IID_TOOL_BAR);
    auto* renderer = queryInterface<IRenderer>(IID_RENDERER);

    auto* yasha_manager = static_cast<ToolManager*>(manager);
    if (document == nullptr ||
        desktop == nullptr ||
        yasha_manager == nullptr ||
        toolbar == nullptr ||
        renderer == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL,
                             "Selector tool requires Desktop, Document, ToolManager, ToolBar and Render");
        return false;
    }

    const auto texture = renderer->loadTexture("SelectorTool/assets/selector.png");

    if (!texture)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Selector tool icon could not be loaded");
        return false;
    }

    auto&      state       = registration().state();
    auto       overlay     = std::make_unique<Yasha::Tools::SelectionOverlay>(*document, *yasha_manager,
                                                                              Yasha::Tools::SELECTOR_TOOL_ID, state);
    const auto overlay_id  = overlay->id();
    auto*      overlay_ptr = overlay.get();
    if (desktop->add(std::move(overlay)) != IDesktop::Error::SUCCESS)
    {
        return false;
    }

    auto tool = std::make_unique<Yasha::Tools::SelectorTool>(*document, *yasha_manager, *overlay_ptr, state);
    if (manager->addTool(std::move(tool)) != IToolManager::Error::SUCCESS)
    {
        desktop->remove(overlay_id);
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Selector tool could not be registered");
        return false;
    }
    registration().setManager(manager);

    toolbar->addTool(Yasha::Tools::SELECTOR_TOOL_ID, *texture);
    if (manager->setActiveTool(Yasha::Tools::SELECTOR_TOOL_ID) != IToolManager::Error::SUCCESS)
    {
        toolbar->removeTool(Yasha::Tools::SELECTOR_TOOL_ID);
        manager->removeTool(Yasha::Tools::SELECTOR_TOOL_ID);
        registration().setManager(nullptr);
        desktop->remove(overlay_id);
        return false;
    }
    return true;
}
