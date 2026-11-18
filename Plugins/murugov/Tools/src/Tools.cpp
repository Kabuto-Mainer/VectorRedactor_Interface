#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDesktop.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/IToolBar.hpp"
#include "Interfaces/IToolManager.hpp"
#include "Interfaces/Package.hpp"
#include "SelectTool.hpp"
#include "Utils/TObject.hpp"
#include "Utils/Vec2F.hpp"

#include <memory>
#include <optional>

namespace
{

constexpr const char* SELECT_TOOL_ICON = "assets/arrow.png";

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

class SelectOverlay final : public TObject
{
public:
    explicit SelectOverlay(Murugov::Tools::SelectTool& tool) : tool_(tool)
    {
    }

    bool processEvent(const IInputPump::Event& event) override
    {
        static_cast<void>(event);
        return false;
    }

    void render() override
    {
        tool_.render();
    }

    Vec2F mapLocalToGlobal(const Vec2F& pos) const override
    {
        return parent() != nullptr ? parent()->mapLocalToGlobal(pos) : pos;
    }

    Vec2F mapGlobalToLocal(const Vec2F& pos) const override
    {
        return parent() != nullptr ? parent()->mapGlobalToLocal(pos) : pos;
    }

private:
    Murugov::Tools::SelectTool& tool_;
}; // class SelectOverlay

} // namespace

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    IDocument*    document     = queryInterface<IDocument>(IID_IDOCUMENT);
    IToolManager* tool_manager = queryInterface<IToolManager>(IID_TOOL_MANAGER);
    IToolBar*     tool_bar     = queryInterface<IToolBar>(IID_TOOL_BAR);
    IRenderer*    renderer     = queryInterface<IRenderer>(IID_RENDERER);
    IDesktop*     desktop      = queryInterface<IDesktop>(IID_DESKTOP);

    if (document == nullptr ||
        tool_manager == nullptr ||
        tool_bar == nullptr ||
        renderer == nullptr ||
        desktop == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Murugov::Tools: missing required interfaces");
        return false;
    }

    auto select_tool =
        std::make_unique<Murugov::Tools::SelectTool>(Murugov::Tools::SELECT_TOOL_ID, *document, *renderer);
    auto* select_tool_raw = select_tool.get();

    if (tool_manager->addTool(std::move(select_tool)) != IToolManager::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Murugov::Tools: failed to register SelectTool");
        return false;
    }

    const std::optional<IRenderer::TextureID> icon = renderer->loadTexture(SELECT_TOOL_ICON);
    if (!icon)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "Murugov::Tools: can't load the icon '%s'",
                             SELECT_TOOL_ICON);
    }
    else
    {
        tool_bar->addTool(Murugov::Tools::SELECT_TOOL_ID, *icon);
    }

    auto overlay = std::make_unique<SelectOverlay>(*select_tool_raw);
    if (desktop->add(std::move(overlay)) != IDesktop::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Murugov::Tools: failed to add the overlay");
        return false;
    }

    return true;
}
