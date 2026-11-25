#include "RectTool.hpp"

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDesktop.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/IToolBar.hpp"
#include "Interfaces/IToolManager.hpp"
#include "Interfaces/Package.hpp"
#include "Utils/TWidget.hpp"

#include <memory>

extern "C" InitPackageFunc INIT_PACKAGE_FUNC_NAME;

namespace Murugov
{

namespace
{

class RectOverlayWidget final : public TWidget
{
public:
    explicit RectOverlayWidget(Tools::RectTool& tool) : tool_(tool) {}

    bool processEvent(const IInputPump::Event& /*event*/) override
    {
        return false;
    }

    void render() override
    {
        tool_.render();
    }

private:
    Tools::RectTool& tool_;
};

} // namespace

class RectToolPlugin final
{
private:
    bool        initialized_ = false;
    TObject::ID overlay_id_  = 0;

    RectToolPlugin()
    {
        auto& core = ApplicationCore::instance();

        IDocument*    document     = nullptr;
        IRenderer*    renderer     = nullptr;
        IDesktop*     desktop      = nullptr;
        IToolManager* tool_manager = nullptr;
        IToolBar*     tool_bar     = nullptr;

        if (core.queryInterface(IID_IDOCUMENT, reinterpret_cast<void**>(&document)) != ApplicationCore::Error::SUCCESS ||
            document == nullptr)
        {
            ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "RectTool requires Document");
            return;
        }

        if (core.queryInterface(IID_RENDERER, reinterpret_cast<void**>(&renderer)) != ApplicationCore::Error::SUCCESS ||
            renderer == nullptr)
        {
            ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "RectTool requires Renderer");
            return;
        }

        if (core.queryInterface(IID_DESKTOP, reinterpret_cast<void**>(&desktop)) != ApplicationCore::Error::SUCCESS ||
            desktop == nullptr)
        {
            ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "RectTool requires Desktop");
            return;
        }

        if (core.queryInterface(IID_TOOL_MANAGER, reinterpret_cast<void**>(&tool_manager)) !=
                ApplicationCore::Error::SUCCESS ||
            tool_manager == nullptr)
        {
            ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "RectTool requires ToolManager");
            return;
        }

        if (core.queryInterface(IID_TOOL_BAR, reinterpret_cast<void**>(&tool_bar)) != ApplicationCore::Error::SUCCESS ||
            tool_bar == nullptr)
        {
            ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "RectTool requires ToolBar");
            return;
        }

        auto texture = renderer->loadTexture("../../murugov/Tools/RectTool/assets/rect.png");
        if (!texture.has_value())
        {
            ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "RectTool failed to load icon texture");
            return;
        }

        auto  tool     = std::make_unique<Tools::RectTool>(Tools::RECT_TOOL_ID, *document, *renderer);
        auto* tool_ptr = tool.get();

        auto overlay = std::make_unique<RectOverlayWidget>(*tool_ptr);
        overlay_id_  = overlay->id();

        if (desktop->add(std::move(overlay)) != IDesktop::Error::SUCCESS)
        {
            overlay_id_ = 0;
            return;
        }

        if (tool_manager->addTool(std::move(tool)) != IToolManager::Error::SUCCESS)
        {
            desktop->remove(overlay_id_);
            overlay_id_ = 0;
            return;
        }

        tool_bar->addTool(Tools::RECT_TOOL_ID, *texture);
        initialized_ = true;
    }

    ~RectToolPlugin()
    {
        if (!initialized_)
        {
            return;
        }

        auto& core = ApplicationCore::instance();

        IToolBar* tool_bar = nullptr;
        if (core.queryInterface(IID_TOOL_BAR, reinterpret_cast<void**>(&tool_bar)) == ApplicationCore::Error::SUCCESS &&
            tool_bar != nullptr)
        {
            tool_bar->removeTool(Tools::RECT_TOOL_ID);
        }

        IToolManager* tool_manager = nullptr;
        if (core.queryInterface(IID_TOOL_MANAGER, reinterpret_cast<void**>(&tool_manager)) ==
                ApplicationCore::Error::SUCCESS &&
            tool_manager != nullptr)
        {
            tool_manager->removeTool(Tools::RECT_TOOL_ID);
        }

        IDesktop* desktop = nullptr;
        if (overlay_id_ != 0 &&
            core.queryInterface(IID_DESKTOP, reinterpret_cast<void**>(&desktop)) == ApplicationCore::Error::SUCCESS &&
            desktop != nullptr)
        {
            desktop->remove(overlay_id_);
        }
    }

public:
    RectToolPlugin(const RectToolPlugin&)            = delete;
    RectToolPlugin(RectToolPlugin&&)                 = delete;
    RectToolPlugin& operator=(const RectToolPlugin&) = delete;
    RectToolPlugin& operator=(RectToolPlugin&&)      = delete;

    static RectToolPlugin& instance()
    {
        static RectToolPlugin inst;
        return inst;
    }

    bool isValid() const { return initialized_; }
};

} // namespace Murugov

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    return Murugov::RectToolPlugin::instance().isValid();
}