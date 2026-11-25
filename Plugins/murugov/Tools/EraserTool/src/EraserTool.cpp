#include "EraserTool.hpp"

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDesktop.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/IToolBar.hpp"
#include "Interfaces/IToolManager.hpp"
#include "Interfaces/Package.hpp"
#include "Utils/TWidget.hpp"

#include <memory>
#include <variant>

extern "C" InitPackageFunc INIT_PACKAGE_FUNC_NAME;

namespace Murugov
{

namespace
{

class EraserOverlayWidget final : public TWidget
{
public:
    explicit EraserOverlayWidget(Tools::EraserTool& tool) : tool_(tool) {}

    bool processEvent(const IInputPump::Event& event) override
    {
        if (const auto* mouse = std::get_if<IInputPump::EventMouse>(&event))
        {
            if (mouse->type() == IInputPump::EventMouse::Type::LEFT_BUTTON_PRESSING)
            {
                tool_.deactivate();
            }
        }
        return false;
    }

    void render() override
    {
        tool_.render();
    }

private:
    Tools::EraserTool& tool_;
};

} // namespace

class EraserToolPlugin final
{
private:
    bool        initialized_ = false;
    TObject::ID overlay_id_  = 0;

    EraserToolPlugin()
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
            ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "EraserTool requires Document");
            return;
        }

        if (core.queryInterface(IID_RENDERER, reinterpret_cast<void**>(&renderer)) != ApplicationCore::Error::SUCCESS ||
            renderer == nullptr)
        {
            ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "EraserTool requires Renderer");
            return;
        }

        if (core.queryInterface(IID_DESKTOP, reinterpret_cast<void**>(&desktop)) != ApplicationCore::Error::SUCCESS ||
            desktop == nullptr)
        {
            ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "EraserTool requires Desktop");
            return;
        }

        if (core.queryInterface(IID_TOOL_MANAGER, reinterpret_cast<void**>(&tool_manager)) !=
                ApplicationCore::Error::SUCCESS ||
            tool_manager == nullptr)
        {
            ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "EraserTool requires ToolManager");
            return;
        }

        if (core.queryInterface(IID_TOOL_BAR, reinterpret_cast<void**>(&tool_bar)) != ApplicationCore::Error::SUCCESS ||
            tool_bar == nullptr)
        {
            ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "EraserTool requires ToolBar");
            return;
        }

        auto texture = renderer->loadTexture("../../murugov/Tools/EraserTool/assets/eraser.png");
        if (!texture.has_value())
        {
            ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "EraserTool failed to load icon texture");
            return;
        }

        auto  tool     = std::make_unique<Tools::EraserTool>(Tools::ERASER_TOOL_ID, *document, *renderer);
        auto* tool_ptr = tool.get();

        auto overlay = std::make_unique<EraserOverlayWidget>(*tool_ptr);
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

        tool_bar->addTool(Tools::ERASER_TOOL_ID, *texture);
        initialized_ = true;
    }

    ~EraserToolPlugin()
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
            tool_bar->removeTool(Tools::ERASER_TOOL_ID);
        }

        IToolManager* tool_manager = nullptr;
        if (core.queryInterface(IID_TOOL_MANAGER, reinterpret_cast<void**>(&tool_manager)) ==
                ApplicationCore::Error::SUCCESS &&
            tool_manager != nullptr)
        {
            tool_manager->removeTool(Tools::ERASER_TOOL_ID);
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
    EraserToolPlugin(const EraserToolPlugin&)            = delete;
    EraserToolPlugin(EraserToolPlugin&&)                 = delete;
    EraserToolPlugin& operator=(const EraserToolPlugin&) = delete;
    EraserToolPlugin& operator=(EraserToolPlugin&&)      = delete;

    static EraserToolPlugin& instance()
    {
        static EraserToolPlugin inst;
        return inst;
    }

    bool isValid() const { return initialized_; }
};

} // namespace Murugov

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    return Murugov::EraserToolPlugin::instance().isValid();
}