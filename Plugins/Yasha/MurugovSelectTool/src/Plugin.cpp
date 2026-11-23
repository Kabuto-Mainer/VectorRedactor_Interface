#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDesktop.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/IToolBar.hpp"
#include "Interfaces/IToolManager.hpp"
#include "Interfaces/Package.hpp"
#include "QueryInterface.hpp"
#include "SelectTool.hpp"
#include "ToolBarIID.hpp"
#include "ToolManager.hpp"

#include <memory>

namespace
{

class SelectOverlay final : public TObject
{
public:
    SelectOverlay(ToolManager& manager, Murugov::Tools::SelectTool& tool) : manager_(manager), tool_(tool)
    {
    }

    bool processEvent(const IInputPump::Event&) override
    {
        return false;
    }
    Vec2F mapLocalToGlobal(const Vec2F& pos) const override
    {
        return parent() ? parent()->mapLocalToGlobal(pos) : pos;
    }
    Vec2F mapGlobalToLocal(const Vec2F& pos) const override
    {
        return parent() ? parent()->mapGlobalToLocal(pos) : pos;
    }
    void render() override
    {
        if (manager_.activeToolId() == Murugov::Tools::SELECT_TOOL_ID)
        {
            tool_.render();
        }
    }

private:
    ToolManager&                manager_;
    Murugov::Tools::SelectTool& tool_;
};

class ToolRegistration final
{
public:
    ~ToolRegistration()
    {
        if (manager_ != nullptr)
        {
            manager_->removeTool(Murugov::Tools::SELECT_TOOL_ID);
        }
    }

    void setManager(IToolManager* manager)
    {
        manager_ = manager;
    }

private:
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
    auto* manager  = queryInterface<IToolManager>(IID_TOOL_MANAGER);
    auto* renderer = queryInterface<IRenderer>(IID_RENDERER);
    auto* desktop  = queryInterface<IDesktop>(IID_DESKTOP);
    auto* toolbar  = queryInterface<IToolBar>(IID_TOOL_BAR);
    if (document == nullptr || manager == nullptr || renderer == nullptr || desktop == nullptr || toolbar == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL,
                             "Murugov SelectTool requires Document, ToolManager, Render, Desktop and ToolBar");
        return false;
    }

    const auto texture = renderer->loadTexture("MurugovSelectTool/assets/select.png");
    if (!texture)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Murugov SelectTool icon could not be loaded");
        return false;
    }

    // The current toolbar occupies this area; its own event handler consumes clicks first.
    const BoxF ui_box{16.0F, 16.0F, 56.0F, 176.0F};
    auto       tool =
        std::make_unique<Murugov::Tools::SelectTool>(Murugov::Tools::SELECT_TOOL_ID, *document, *renderer, ui_box);
    auto* tool_ptr = tool.get();
    if (manager->addTool(std::move(tool)) != IToolManager::Error::SUCCESS)
    {
        return false;
    }

    auto       overlay    = std::make_unique<SelectOverlay>(*static_cast<ToolManager*>(manager), *tool_ptr);
    const auto overlay_id = overlay->id();
    if (desktop->add(std::move(overlay)) != IDesktop::Error::SUCCESS)
    {
        manager->removeTool(Murugov::Tools::SELECT_TOOL_ID);
        return false;
    }

    if (manager->setActiveTool(Murugov::Tools::SELECT_TOOL_ID) != IToolManager::Error::SUCCESS)
    {
        desktop->remove(overlay_id);
        manager->removeTool(Murugov::Tools::SELECT_TOOL_ID);
        return false;
    }

    toolbar->addTool(Murugov::Tools::SELECT_TOOL_ID, *texture);
    registration().setManager(manager);
    return true;
}
