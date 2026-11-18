#include "Core/ApplicationCore.hpp"
#include "IUIFramework.hpp"
#include "Interfaces/IDesktop.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/IToolManager.hpp"
#include "Interfaces/Package.hpp"
#include "SelectTool.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/TObject.hpp"
#include "Utils/Vec2F.hpp"

#include <memory>
#include <string>

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

class SelectOverlay final : public TObject
{
public:
    SelectOverlay(bool& is_active, Murugov::Tools::SelectTool& tool) : is_active_(is_active), tool_(tool) {}

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
        if (is_active_)
        {
            tool_.render();
        }
    }

private:
    bool&                       is_active_;
    Murugov::Tools::SelectTool& tool_;
};

inline BoxF murugovToolBarBox()
{
    return BoxF(508.0F, 24.0F, 648.0F, 100.0F);
}

} // namespace

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    IDocument*    document     = queryInterface<IDocument>(IID_IDOCUMENT);
    IToolManager* tool_manager = queryInterface<IToolManager>(IID_TOOL_MANAGER);
    IRenderer*    renderer     = queryInterface<IRenderer>(IID_RENDERER);
    IUIFramework* framework    = queryInterface<IUIFramework>(IID_IUIFramework);
    IDesktop*     desktop      = queryInterface<IDesktop>(IID_DESKTOP);

    if (!document || !tool_manager || !renderer || !framework || !desktop)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Murugov::Tools: missing required interfaces");
        return false;
    }

    static bool is_select_active = false;

    const BoxF ui_box(312.0F, 24.0F, 648.0F, 228.0F);

    auto select_tool = std::make_unique<Murugov::Tools::SelectTool>(
        Murugov::Tools::SELECT_TOOL_ID, *document, *renderer, ui_box
    );
    auto* select_tool_raw = select_tool.get();

    if (tool_manager->addTool(std::move(select_tool)) != IToolManager::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Murugov::Tools: failed to register SelectTool");
        return false;
    }

    auto overlay = std::make_unique<SelectOverlay>(is_select_active, *select_tool_raw);
    if (desktop->add(std::move(overlay)) != IDesktop::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Murugov::Tools: failed to add overlay");
        return false;
    }

    const BoxF bar_box = murugovToolBarBox();

    IUIFramework::WindowMenuDescription win_desc;
    win_desc.box   = bar_box;
    win_desc.label = std::string("Murugov");
    win_desc.role  = IUIFramework::Role::DEFAULT;

    auto window = framework->create(win_desc);
    if (!window)
    {
        return false;
    }

    IUIFramework::SwitcherPoolDescription pool_desc;
    pool_desc.min_point = Vec2F(8.0F, 36.0F);
    pool_desc.size      = Vec2F(bar_box.size().x() - 16.0F, 32.0F);
    pool_desc.direction = IUIFramework::Direction::VERTICAL;
    pool_desc.role      = IUIFramework::Role::DEFAULT;

    auto pool = framework->create(pool_desc);
    if (!pool)
    {
        return false;
    }

    IUIFramework::SwitcherDescription btn_desc;
    btn_desc.text = std::string("Select");
    btn_desc.role = IUIFramework::Role::DEFAULT;
    btn_desc.on_active.callbacks.emplace_back([tool_manager] {
        is_select_active = true;
        tool_manager->setActiveTool(Murugov::Tools::SELECT_TOOL_ID);
    });
    btn_desc.on_deactive.callbacks.emplace_back([] {
        is_select_active = false;
    });

    auto btn = framework->create(btn_desc);
    if (!btn)
    {
        return false;
    }

    pool->add(std::move(btn));
    window->add(std::move(pool));

    if (desktop->add(std::move(window)) != IDesktop::Error::SUCCESS)
    {
        return false;
    }

    return true;
}
