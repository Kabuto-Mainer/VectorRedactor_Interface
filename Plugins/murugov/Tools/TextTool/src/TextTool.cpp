#include "TextTool.hpp"

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

class TextOverlayWidget final : public TWidget
{
public:
    explicit TextOverlayWidget(Tools::TextTool& tool) : tool_(tool) {}

    bool processEvent(const IInputPump::Event& event) override
    {
        if (const auto* mouse = std::get_if<IInputPump::EventMouse>(&event))
        {
            if (mouse->type() == IInputPump::EventMouse::Type::LEFT_BUTTON_PRESSING)
            {
                const Vec2F pos = mouse->cursorPos();
                if (pos.x() < 90.0F)                    // FIXME: to avoid a conflict with clicking on the toolbar
                {
                    tool_.finishEditing();
                }
            }
        }
        return false;
    }

    void render() override
    {
        tool_.render();
    }

private:
    Tools::TextTool& tool_;
};

} // namespace

class TextToolPlugin final
{
private:
    bool        initialized_ = false;
    TObject::ID overlay_id_  = 0;

    TextToolPlugin()
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
            ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "TextTool requires Document");
            return;
        }

        if (core.queryInterface(IID_RENDERER, reinterpret_cast<void**>(&renderer)) != ApplicationCore::Error::SUCCESS ||
            renderer == nullptr)
        {
            ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "TextTool requires Renderer");
            return;
        }

        if (core.queryInterface(IID_DESKTOP, reinterpret_cast<void**>(&desktop)) != ApplicationCore::Error::SUCCESS ||
            desktop == nullptr)
        {
            ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "TextTool requires Desktop");
            return;
        }

        if (core.queryInterface(IID_TOOL_MANAGER, reinterpret_cast<void**>(&tool_manager)) !=
                ApplicationCore::Error::SUCCESS ||
            tool_manager == nullptr)
        {
            ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "TextTool requires ToolManager");
            return;
        }

        if (core.queryInterface(IID_TOOL_BAR, reinterpret_cast<void**>(&tool_bar)) != ApplicationCore::Error::SUCCESS ||
            tool_bar == nullptr)
        {
            ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "TextTool requires ToolBar");
            return;
        }

        auto texture = renderer->loadTexture("../Plugins/murugov/Tools/TextTool/assets/text.png");
        if (!texture.has_value())
        {
            ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "TextTool failed to load icon texture");
            return;
        }

        auto font = renderer->loadFont("/System/Library/Fonts/Supplemental/Arial.ttf");
    
        if (!font.has_value())
        {
            ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "TextTool failed to load font");
            return;
        }

        auto  tool     = std::make_unique<Tools::TextTool>(Tools::TEXT_TOOL_ID, *document, *renderer, *font);
        auto* tool_ptr = tool.get();

        auto overlay = std::make_unique<TextOverlayWidget>(*tool_ptr);
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

        tool_bar->addTool(Tools::TEXT_TOOL_ID, *texture);
        initialized_ = true;
    }

    ~TextToolPlugin()
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
            tool_bar->removeTool(Tools::TEXT_TOOL_ID);
        }

        IToolManager* tool_manager = nullptr;
        if (core.queryInterface(IID_TOOL_MANAGER, reinterpret_cast<void**>(&tool_manager)) ==
                ApplicationCore::Error::SUCCESS &&
            tool_manager != nullptr)
        {
            tool_manager->removeTool(Tools::TEXT_TOOL_ID);
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
    TextToolPlugin(const TextToolPlugin&)            = delete;
    TextToolPlugin(TextToolPlugin&&)                 = delete;
    TextToolPlugin& operator=(const TextToolPlugin&) = delete;
    TextToolPlugin& operator=(TextToolPlugin&&)      = delete;

    static TextToolPlugin& instance()
    {
        static TextToolPlugin inst;
        return inst;
    }

    bool isValid() const { return initialized_; }
};

} // namespace Murugov

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    return Murugov::TextToolPlugin::instance().isValid();
}