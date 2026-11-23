#include "AddShapeTool.hpp"
#include "Core/ApplicationCore.hpp"
#include "DeleteShapeTool.hpp"
#include "Interfaces/IDesktop.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/IToolBar.hpp"
#include "Interfaces/IToolManager.hpp"
#include "Interfaces/Package.hpp"
#include "SetColorTool.hpp"
#include "ToolIds.hpp"
#include "Utils/ACommand.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/TObject.hpp"
#include "Utils/TWidget.hpp"
#include "Utils/Vec2F.hpp"
#include "export/EllipseShape.hpp"
#include "export/IPalette.hpp"
#include "export/IUIFramework.hpp"
#include "export/RectangleShape.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace
{

constexpr float MENU_ITEM_WIDTH  = 140.0F;
constexpr float MENU_ITEM_HEIGHT = 28.0F;
constexpr float CENTER_FACTOR    = 0.5F;

constexpr const char* RECTANGLE_ICON = "assets/rectangle.png";
constexpr const char* ELLIPSE_ICON   = "assets/ellipse.png";
constexpr const char* DELETE_ICON    = "assets/pen.png";
constexpr const char* COLOR_ICON     = "assets/ellipse.png";

struct ToolBarItem
{
    IToolManager::ATool::ID tool_id;
    const char*             icon;
}; // struct ToolBarItem

constexpr ToolBarItem TOOL_BAR_ITEMS[] = {
    {   Kabuto::Tools::ADD_SHAPE_TOOL_ID, RECTANGLE_ICON},
    {Kabuto::Tools::DELETE_SHAPE_TOOL_ID,    DELETE_ICON},
    {   Kabuto::Tools::SET_COLOR_TOOL_ID,     COLOR_ICON},
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

class RemoveObjectCommand final : public ACommand
{
public:
    explicit RemoveObjectCommand(TObject::ID object_id) : object_id_(object_id)
    {
    }

    void execute() const override
    {
        ApplicationCore::instance().removeObject(object_id_);
    }

private:
    TObject::ID object_id_;
}; // class RemoveObjectCommand

void destroyAfterEvent(TObject::ID object_id)
{
    ApplicationCore::instance().pushCommand(std::make_unique<RemoveObjectCommand>(object_id));
}

BoxF widgetGlobalBounds(const TWidget& widget)
{
    BoxF bounds{widget.box().size()};
    bounds.setCenter(widget.mapLocalToGlobal(widget.box().size() * CENTER_FACTOR));
    return bounds;
}

struct ShapeMenuState
{
    TWidget*                    widget = nullptr;
    TObject::ID                 id     = 0;
    std::vector<const TWidget*> items;
}; // struct ShapeMenuState

bool insideMenuItem(const ShapeMenuState& state, const Vec2F& pos)
{
    return std::ranges::any_of(state.items, [&pos](const TWidget* item)
                               { return item != nullptr && widgetGlobalBounds(*item).contains(pos); });
}

void showShapeMenu(Kabuto::IUIFramework& framework, IDesktop& desktop, const Vec2F& anchor,
                   const std::vector<Kabuto::Tools::AddShapeTool::ShapeOption>& options,
                   Kabuto::Tools::AddShapeTool::ChosenHandler                   on_chosen)
{
    auto state = std::make_shared<ShapeMenuState>();

    Kabuto::IUIFramework::ContextMenuDescription description;
    description.min_point = anchor;
    description.direction = Kabuto::IUIFramework::Direction::VERTICAL;
    description.role      = Kabuto::IUIFramework::Role::DEFAULT;
    description.on_event  = [state](const IInputPump::Event& event) -> bool
    {
        const auto* mouse = std::get_if<IInputPump::EventMouse>(&event);
        if (mouse == nullptr || state->widget == nullptr)
        {
            return false;
        }

        const bool outside = !widgetGlobalBounds(*state->widget).contains(mouse->cursorPos());

        using enum IInputPump::EventMouse::Type;

        switch (mouse->type())
        {
            case LEFT_BUTTON_PRESSING:
                if (outside)
                {
                    destroyAfterEvent(state->id);
                    return true;
                }

                /* A press on the menu padding must not leak to the active tool. */
                return !insideMenuItem(*state, mouse->cursorPos());

            case RIGHT_BUTTON_PRESSING:
            case MIDDLE_BUTTON_PRESSING:
                if (outside)
                {
                    destroyAfterEvent(state->id);
                }
                return true;

            default:
                return false;
        }
    };

    std::unique_ptr<Kabuto::IUIFramework::ContextMenu> menu = framework.create(description);
    if (!menu)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Add shape tool can't create the shape menu");
        return;
    }

    state->widget = menu.get();
    state->id     = menu->id();

    for (std::size_t index = 0; index < options.size(); ++index)
    {
        Kabuto::IUIFramework::ButtonDescription button;
        button.box = BoxF{
            Vec2F{MENU_ITEM_WIDTH, MENU_ITEM_HEIGHT}
        };
        button.text = options[index].label;
        if (options[index].icon.has_value())
        {
            button.icon = Kabuto::IUIFramework::Icon{*options[index].icon};
        }
        button.role = Kabuto::IUIFramework::Role::DEFAULT;
        button.on_press.callbacks.emplace_back(
            [on_chosen, state, index]
            {
                on_chosen(index);
                destroyAfterEvent(state->id);
            }
        );

        std::unique_ptr<Kabuto::IUIFramework::Button> item = framework.create(button);
        if (!item)
        {
            ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Add shape tool can't create a shape menu item");
            continue;
        }

        state->items.push_back(item.get());
        menu->add(std::move(item));
    }

    if (desktop.add(std::move(menu)) != IDesktop::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR,
                             "Add shape tool can't add the shape menu to the desktop");
        state->widget = nullptr;
        state->id     = 0;
    }
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
    { return std::make_unique<Kabuto::Document::RectangleShape>(size); };
}

Kabuto::Tools::AddShapeTool::Factory ellipseFactory()
{
    return [](const Vec2F& size) -> std::unique_ptr<IDocument::TShape>
    { return std::make_unique<Kabuto::Document::EllipseShape>(size); };
}

std::unique_ptr<Kabuto::Tools::AddShapeTool> makeAddShapeTool(IDocument& document, Kabuto::IPalette& palette,
                                                              Kabuto::IUIFramework& framework, IDesktop& desktop,
                                                              IRenderer& renderer)
{
    const std::optional<IRenderer::TextureID> rectangle_icon = loadIcon(renderer, RECTANGLE_ICON);
    const std::optional<IRenderer::TextureID> ellipse_icon   = loadIcon(renderer, ELLIPSE_ICON);

    std::vector<Kabuto::Tools::AddShapeTool::ShapeOption> options;
    options.push_back(Kabuto::Tools::AddShapeTool::ShapeOption{"Rectangle", rectangle_icon, rectangleFactory()});
    options.push_back(Kabuto::Tools::AddShapeTool::ShapeOption{"Ellipse", ellipse_icon, ellipseFactory()});

    Kabuto::IUIFramework* framework_ptr = &framework;
    IDesktop*             desktop_ptr   = &desktop;

    return std::make_unique<Kabuto::Tools::AddShapeTool>(
        Kabuto::Tools::ADD_SHAPE_TOOL_ID, document, palette, std::move(options),
        [framework_ptr, desktop_ptr](const Vec2F&                                                 anchor,
                                     const std::vector<Kabuto::Tools::AddShapeTool::ShapeOption>& shape_options,
                                     Kabuto::Tools::AddShapeTool::ChosenHandler                   on_chosen)
        { showShapeMenu(*framework_ptr, *desktop_ptr, anchor, shape_options, std::move(on_chosen)); }
    );
}

} // namespace

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    IDocument*            document     = queryInterface<IDocument>(IID_IDOCUMENT);
    IToolManager*         tool_manager = queryInterface<IToolManager>(IID_TOOL_MANAGER);
    Kabuto::IPalette*     palette      = queryInterface<Kabuto::IPalette>(Kabuto::IID_PALETTE);
    IToolBar*             tool_bar     = queryInterface<IToolBar>(IID_TOOL_BAR);
    IRenderer*            renderer     = queryInterface<IRenderer>(IID_RENDERER);
    Kabuto::IUIFramework* framework    = queryInterface<Kabuto::IUIFramework>(Kabuto::IID_IUIFramework);
    IDesktop*             desktop      = queryInterface<IDesktop>(IID_DESKTOP);

    if (document == nullptr ||
        tool_manager == nullptr ||
        palette == nullptr ||
        tool_bar == nullptr ||
        renderer == nullptr ||
        framework == nullptr ||
        desktop == nullptr)
    {
        ApplicationCore::log(
            ApplicationCore::LogLevel::CRITICAL,
            "Tools can't find the document, the tool manager, the palette, the tool bar, the renderer, the UI "
            "framework or the desktop"
        );
        return false;
    }

    std::unique_ptr<Kabuto::Tools::AddShapeTool> add_shape =
        makeAddShapeTool(*document, *palette, *framework, *desktop, *renderer);

    if (tool_manager->addTool(std::move(add_shape)) != IToolManager::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Tools can't register the add shape tool");
        return false;
    }
    registrationGuard().remember(Kabuto::Tools::ADD_SHAPE_TOOL_ID);

    auto delete_shape =
        std::make_unique<Kabuto::Tools::DeleteShapeTool>(Kabuto::Tools::DELETE_SHAPE_TOOL_ID, *document);
    if (tool_manager->addTool(std::move(delete_shape)) != IToolManager::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Tools can't register the delete shape tool");
        return false;
    }
    registrationGuard().remember(Kabuto::Tools::DELETE_SHAPE_TOOL_ID);

    auto set_color =
        std::make_unique<Kabuto::Tools::SetColorTool>(Kabuto::Tools::SET_COLOR_TOOL_ID, *document, *palette);
    if (tool_manager->addTool(std::move(set_color)) != IToolManager::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Tools can't register the set color tool");
        return false;
    }
    registrationGuard().remember(Kabuto::Tools::SET_COLOR_TOOL_ID);

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
