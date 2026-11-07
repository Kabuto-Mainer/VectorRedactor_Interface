#include "Core/ApplicationCore.hpp"
#include "IUIFramework.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/Package.hpp"
#include "Utils/ACommand.hpp"
#include "Utils/AObject.hpp"
#include "Utils/AWidget.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/Event.hpp"
#include "Utils/Vec2F.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace UI
{

namespace
{

constexpr uint8_t OPAQUE_ALPHA         = 255;
constexpr float   CHANNEL_MAX          = 255.0F;
constexpr float   NO_ANGLE             = 0.0F;
constexpr float   PADDING              = 4.0F;
constexpr float   HEADER_HEIGHT        = 28.0F;
constexpr float   ACCENT_LINE_HEIGHT   = 2.0F;
constexpr float   CLOSE_BUTTON_SIZE    = 20.0F;
constexpr float   CLOSE_AREA_WIDTH     = CLOSE_BUTTON_SIZE + 2.0F * PADDING;
constexpr float   MENU_ITEM_WIDTH      = 120.0F;
constexpr float   MENU_ITEM_HEIGHT     = 28.0F;
constexpr int     FONT_SIZE            = 16;
constexpr char    DEFAULT_THEME_NAME[] = "default";
constexpr char    DARK_THEME_NAME[]    = "dark";

constexpr float LIGHTNESS_THRESHOLD = 0.5F;
constexpr float SECONDARY_TINT      = 0.15F;
constexpr float CANVAS_TINT         = 0.04F;
constexpr float HOVER_MIX           = 0.08F;
constexpr float PRESSED_MIX         = 0.16F;
constexpr float BORDER_MIX          = 0.20F;
constexpr float DARK_SURFACE_LIFT   = 0.08F;
constexpr float LIGHT_SURFACE_LIFT  = 0.50F;

struct Palette
{
    Color base;
    Color hover;
    Color pressed;
    Color text;
    Color border;
    Color surface;
    Color surface_text;
    Color canvas;
};

Color mix(const Color& from, const Color& to, float ratio)
{
    const auto channel = [ratio](uint8_t from_value, uint8_t to_value)
    {
        return static_cast<uint8_t>(static_cast<float>(from_value) +
                                    (static_cast<float>(to_value) - static_cast<float>(from_value)) * ratio);
    };
    return Color{channel(from.r, to.r), channel(from.g, to.g), channel(from.b, to.b), OPAQUE_ALPHA};
}

float luminance(const Color& color)
{
    return (0.2126F * color.r + 0.7152F * color.g + 0.0722F * color.b) / CHANNEL_MAX;
}

Color contrastOf(const Color& color)
{
    if (luminance(color) < LIGHTNESS_THRESHOLD)
    {
        return Color{255, 255, 255, OPAQUE_ALPHA};
    }
    return Color{0, 0, 0, OPAQUE_ALPHA};
}

Palette derive(const Color& base, const Color& surface, const Color& canvas)
{
    const Color text   = contrastOf(base);
    const Color panel  = mix(surface, base, SECONDARY_TINT);
    const Color tinted = mix(canvas, base, CANVAS_TINT);
    return Palette{base,
                   mix(base, text, HOVER_MIX),
                   mix(base, text, PRESSED_MIX),
                   text,
                   mix(base, text, BORDER_MIX),
                   panel,
                   contrastOf(panel),
                   tinted};
}

struct Theme
{
    Color                                                  WindowDescription;
    Color                                                  surface;
    Color                                                  accent;
    std::array<Color, static_cast<std::size_t>(Role::MAX)> role_bases;

    Palette palette(Role role) const;
};

inline Palette Theme::palette(Role role) const
{
    return derive(role_bases[static_cast<std::size_t>(role)], surface, WindowDescription);
}

Theme makeTheme(const Color& primary, const Color& secondary, const Color& neutral, const Color& accent,
                const Color& unical)
{
    const bool  dark = luminance(neutral) < LIGHTNESS_THRESHOLD;
    const Color white{255, 255, 255, OPAQUE_ALPHA};
    const Color surface = mix(neutral, white, dark ? DARK_SURFACE_LIFT : LIGHT_SURFACE_LIFT);
    const Color button  = mix(neutral, secondary, SECONDARY_TINT);

    return Theme{
        neutral, surface, accent, {button, primary, unical}
    };
}

struct RenderContext
{
    const Theme*                     theme = nullptr;
    std::optional<IRenderer::FontID> font;
};

BoxF makeBox(const Vec2F& pos, const Vec2F& size)
{
    return BoxF(pos, pos + size);
}

} // namespace

class UIWi final : public IFramework
{
public:
    UIWi();

    UIWi(const UIWi&)            = delete;
    UIWi& operator=(const UIWi&) = delete;

    std::unique_ptr<AObject> create(ADescription& description) override;

    void registryTheme(std::string name, const Color& primary, const Color& secondary, const Color& neutral,
                       const Color& accent, const Color& unical) override;

    bool setTheme(std::string name) override;

    bool setFont(std::string path) override;

private:
    RenderContext                          context_;
    std::unordered_map<std::string, Theme> themes_;
};

namespace
{

IRenderer* queryRenderer()
{
    IRenderer* renderer = nullptr;
    if (ApplicationCore::instance().queryInterface(IID_RENDERER, reinterpret_cast<void**>(&renderer)) !=
        ApplicationCore::Error::SUCCESS)
    {
        return nullptr;
    }
    return renderer;
}

const EventMouse* asMouse(const Event& event)
{
    return std::get_if<EventMouse>(&event);
}

Role resolveRole(const std::optional<Role>& role)
{
    return role.value_or(Role::Default);
}

std::optional<IRenderer::TextureID> loadIcon(const std::optional<std::string>& path)
{
    IRenderer* renderer = queryRenderer();
    if (!path || !renderer)
    {
        return std::nullopt;
    }
    return renderer->loadTexture(path->c_str());
}

void drawPanel(IRenderer* renderer, const Color& fill, const Color& border, const BoxF& box)
{
    renderer->renderRect(fill, box, NO_ANGLE);
    renderer->renderRectLines(border, box, NO_ANGLE);
}

void drawText(IRenderer* renderer, const RenderContext& context, const std::string& text, const Color& color,
              const BoxF& box)
{
    if (!context.font)
    {
        return;
    }
    renderer->renderText(text.c_str(), color, box, *context.font, FONT_SIZE);
}

void drawContent(IRenderer* renderer, const RenderContext& context, const std::optional<IRenderer::TextureID>& icon,
                 const std::optional<std::string>& text, const Color& color, const BoxF& box)
{
    if (icon)
    {
        renderer->renderTexture(*icon,
                                makeBox(box.topLeft() + Vec2F(PADDING, PADDING), box.size() - Vec2F(2.0F * PADDING)),
                                NO_ANGLE);
    }
    if (text)
    {
        drawText(renderer, context, *text, color, box);
    }
}

Vec2F menuSize(Direction direction, std::size_t count)
{
    const auto items = static_cast<float>(count);
    return direction == Direction::Vertical ? Vec2F(MENU_ITEM_WIDTH, MENU_ITEM_HEIGHT * items) :
                                              Vec2F(MENU_ITEM_WIDTH * items, MENU_ITEM_HEIGHT);
}

using Placement = std::optional<BoxF>;

std::vector<Placement> slotsFor(const Vec2F& size, Direction direction, std::size_t count)
{
    std::vector<Placement> slots;
    if (count == 0)
    {
        return slots;
    }

    const auto  divider = static_cast<float>(count);
    const Vec2F slot =
        direction == Direction::Vertical ? Vec2F(size.x(), size.y() / divider) : Vec2F(size.x() / divider, size.y());
    const Vec2F step = direction == Direction::Vertical ? Vec2F(0.0F, slot.y()) : Vec2F(slot.x(), 0.0F);

    Vec2F position;
    for (std::size_t i = 0; i < count; ++i)
    {
        slots.emplace_back(makeBox(position, slot));
        position = position + step;
    }
    return slots;
}

enum class PressResult
{
    IGNORED,
    CONSUMED,
    CLICKED,
};

class PressTracker
{
public:
    PressResult process(const BoxF& bounds, const Event& event);

    Color background(const Color& idle, const Palette& palette) const;

private:
    bool hovered_ = false;
    bool pressed_ = false;
};

inline PressResult PressTracker::process(const BoxF& bounds, const Event& event)
{
    const EventMouse* mouse = asMouse(event);
    if (!mouse)
    {
        return PressResult::IGNORED;
    }

    const bool inside = bounds.contains(mouse->cursorPos());
    hovered_          = inside;

    switch (mouse->type())
    {
        case EventMouse::Type::LEFT_BUTTON_PRESSING:
            if (inside)
            {
                pressed_ = true;
                return PressResult::CONSUMED;
            }
            return PressResult::IGNORED;

        case EventMouse::Type::LEFT_BUTTON_RELEASING:
            if (!pressed_)
            {
                return PressResult::IGNORED;
            }
            pressed_ = false;
            return inside ? PressResult::CLICKED : PressResult::CONSUMED;

        default:
            return PressResult::IGNORED;
    }
}

inline Color PressTracker::background(const Color& idle, const Palette& palette) const
{
    if (hovered_ && pressed_)
    {
        return palette.pressed;
    }
    if (hovered_)
    {
        return palette.hover;
    }
    return idle;
}

void runAction(Action& action)
{
    for (auto& command : action.actions)
    {
        command->execute();
    }
    for (auto& callback : action.callbacks)
    {
        callback();
    }
}

Vec2F globalCenter(const AWidget& widget)
{
    return widget.mapLocalToGlobal(widget.box().size() / 2.0F);
}

BoxF globalBounds(const AWidget& widget)
{
    BoxF bounds(widget.box().size());
    bounds.setCenter(globalCenter(widget));
    return bounds;
}

class ButtonObject final : public AWidget
{
public:
    ButtonObject(const RenderContext& context, const BoxF& bounds, std::optional<std::string> text,
                 std::optional<IRenderer::TextureID> icon, Role role, Action on_press);

    bool processEvent(const Event& event) override;

    void render() override;

private:
    const RenderContext&                context_;
    std::optional<std::string>          text_;
    std::optional<IRenderer::TextureID> icon_;
    Role                                role_;
    Action                              on_press_;
    PressTracker                        press_;
};

inline ButtonObject::ButtonObject(const RenderContext& context, const BoxF& bounds, std::optional<std::string> text,
                                  std::optional<IRenderer::TextureID> icon, Role role, Action on_press) :
    context_(context), text_(std::move(text)), icon_(icon), role_(role), on_press_(std::move(on_press))
{
    setBox(bounds);
}

inline bool ButtonObject::processEvent(const Event& event)
{
    switch (press_.process(globalBounds(*this), event))
    {
        case PressResult::CLICKED:
            runAction(on_press_);
            return true;
        case PressResult::CONSUMED:
            return true;
        default:
            return false;
    }
}

inline void ButtonObject::render()
{
    IRenderer* renderer = queryRenderer();
    if (!renderer)
    {
        return;
    }

    const Palette palette = context_.theme->palette(role_);
    const BoxF    bounds  = globalBounds(*this);
    drawPanel(renderer, press_.background(palette.base, palette), palette.border, bounds);
    drawContent(renderer, context_, icon_, text_, palette.text, bounds);
}

class SwitcherObject final : public AWidget
{
public:
    SwitcherObject(const RenderContext& context, const BoxF& bounds, std::optional<std::string> text,
                   std::optional<IRenderer::TextureID> icon, Role role, Action on_active, Action on_deactive);

    bool processEvent(const Event& event) override;

    void render() override;

private:
    const RenderContext&                context_;
    std::optional<std::string>          text_;
    std::optional<IRenderer::TextureID> icon_;
    Role                                role_;
    Action                              on_active_;
    Action                              on_deactive_;
    PressTracker                        press_;
    bool                                active_ = false;
};

inline SwitcherObject::SwitcherObject(const RenderContext& context, const BoxF& bounds, std::optional<std::string> text,
                                      std::optional<IRenderer::TextureID> icon, Role role, Action on_active,
                                      Action on_deactive) :
    context_(context), text_(std::move(text)), icon_(icon), role_(role), on_active_(std::move(on_active)),
    on_deactive_(std::move(on_deactive))
{
    setBox(bounds);
}

inline bool SwitcherObject::processEvent(const Event& event)
{
    switch (press_.process(globalBounds(*this), event))
    {
        case PressResult::CLICKED:
            active_ = !active_;
            runAction(active_ ? on_active_ : on_deactive_);
            return true;
        case PressResult::CONSUMED:
            return true;
        default:
            return false;
    }
}

inline void SwitcherObject::render()
{
    IRenderer* renderer = queryRenderer();
    if (!renderer)
    {
        return;
    }

    const Palette palette = context_.theme->palette(role_);
    const BoxF    bounds  = globalBounds(*this);
    const Color   idle    = active_ ? palette.base : palette.surface;
    const Color   text    = active_ ? palette.text : palette.surface_text;
    const Color   border  = active_ ? context_.theme->accent : palette.border;
    drawPanel(renderer, press_.background(idle, palette), border, bounds);
    drawContent(renderer, context_, icon_, text_, text, bounds);
}

class ContextMenuDescriptionObject final : public AWidget
{
public:
    ContextMenuDescriptionObject(const RenderContext& context, const BoxF& bounds, Role role,
                                 std::optional<EventHandler> on_event);

    bool processEvent(const Event& event) override;

    void render() override;

private:
    bool isPressOutside(const Event& event) const;

    const RenderContext&        context_;
    Role                        role_;
    std::optional<EventHandler> on_event_;
    bool                        visible_ = true;
};

inline ContextMenuDescriptionObject::ContextMenuDescriptionObject(const RenderContext& context, const BoxF& bounds,
                                                                  Role role, std::optional<EventHandler> on_event) :
    context_(context), role_(role), on_event_(std::move(on_event))
{
    setBox(bounds);
}

inline bool ContextMenuDescriptionObject::processEvent(const Event& event)
{
    if (!visible_)
    {
        return false;
    }

    if (on_event_)
    {
        if ((*on_event_)(event))
        {
            return true;
        }
    }
    else if (isPressOutside(event))
    {
        visible_ = false;
    }
    return processEventChildren(event);
}

inline void ContextMenuDescriptionObject::render()
{
    IRenderer* renderer = queryRenderer();
    if (!visible_ || !renderer)
    {
        return;
    }

    const Palette palette = context_.theme->palette(role_);
    drawPanel(renderer, palette.surface, palette.border, globalBounds(*this));
    renderChildren();
}

inline bool ContextMenuDescriptionObject::isPressOutside(const Event& event) const
{
    const EventMouse* mouse = asMouse(event);
    return mouse &&
           mouse->type() == EventMouse::Type::LEFT_BUTTON_PRESSING &&
           !globalBounds(*this).contains(mouse->cursorPos());
}

class PoolMenuDescriptionObject final : public AWidget
{
public:
    PoolMenuDescriptionObject(const RenderContext& context, const BoxF& bounds, Role role);

    bool processEvent(const Event& event) override;

    void render() override;

private:
    const RenderContext& context_;
    Role                 role_;
};

inline PoolMenuDescriptionObject::PoolMenuDescriptionObject(const RenderContext& context, const BoxF& bounds,
                                                            Role role) : context_(context), role_(role)
{
    setBox(bounds);
}

inline bool PoolMenuDescriptionObject::processEvent(const Event& event)
{
    return processEventChildren(event);
}

inline void PoolMenuDescriptionObject::render()
{
    IRenderer* renderer = queryRenderer();
    if (!renderer)
    {
        return;
    }

    const Palette palette = context_.theme->palette(role_);
    drawPanel(renderer, palette.surface, palette.border, globalBounds(*this));
    renderChildren();
}

class WindowDescriptionObject final : public AWidget
{
public:
    WindowDescriptionObject(const RenderContext& context, const BoxF& bounds, std::optional<std::string> label,
                            Role role, std::optional<EventHandler> on_event);

    bool processEvent(const Event& event) override;

    void render() override;

private:
    const RenderContext&        context_;
    std::optional<std::string>  label_;
    Role                        role_;
    std::optional<EventHandler> on_event_;
};

inline WindowDescriptionObject::WindowDescriptionObject(const RenderContext& context, const BoxF& bounds,
                                                        std::optional<std::string> label, Role role,
                                                        std::optional<EventHandler> on_event) :
    context_(context), label_(std::move(label)), role_(role), on_event_(std::move(on_event))
{
    setBox(bounds);
}

inline bool WindowDescriptionObject::processEvent(const Event& event)
{
    if (on_event_ && (*on_event_)(event))
    {
        return true;
    }
    return processEventChildren(event);
}

inline void WindowDescriptionObject::render()
{
    IRenderer* renderer = queryRenderer();
    if (!renderer)
    {
        return;
    }

    const Palette palette    = context_.theme->palette(role_);
    const BoxF    bounds     = globalBounds(*this);
    const Vec2F   origin     = bounds.topLeft();
    const BoxF    header_box = makeBox(origin, Vec2F(bounds.size().x(), HEADER_HEIGHT));

    drawPanel(renderer, palette.canvas, palette.border, bounds);
    drawPanel(renderer, palette.base, palette.border, header_box);
    renderer->renderRect(context_.theme->accent,
                         makeBox(Vec2F(origin.x(), origin.y() + HEADER_HEIGHT - ACCENT_LINE_HEIGHT),
                                 Vec2F(bounds.size().x(), ACCENT_LINE_HEIGHT)),
                         NO_ANGLE);

    if (label_)
    {
        const BoxF label_box =
            makeBox(header_box.topLeft(), Vec2F(header_box.size().x() - CLOSE_AREA_WIDTH, HEADER_HEIGHT));
        drawText(renderer, context_, *label_, palette.text, label_box);
    }

    renderChildren();
}

class Builder final : public ADescriptionVisitor
{
public:
    Builder(const RenderContext& context, Placement placement);

    std::unique_ptr<AObject> take();

    bool visitButton(ADescription& description) override;
    bool visitSwitcher(ADescription& description) override;
    bool visitContextMenu(ADescription& description) override;
    bool visitPoolMenu(ADescription& description) override;
    bool visitWindow(ADescription& description) override;

private:
    bool addChildren(AObject& parent, std::vector<std::unique_ptr<ADescription>>& objects,
                     const std::vector<Placement>& placements);

    const RenderContext&     context_;
    Placement                placement_;
    std::unique_ptr<AObject> result_;
};

inline Builder::Builder(const RenderContext& context, Placement placement) : context_(context), placement_(placement)
{
}

inline std::unique_ptr<AObject> Builder::take()
{
    return std::move(result_);
}

inline bool Builder::visitButton(ADescription& description)
{
    auto& button = static_cast<ButtonDescription&>(description);
    result_ =
        std::make_unique<ButtonObject>(context_, placement_.value_or(button.box), std::move(button.text),
                                       loadIcon(button.icon), resolveRole(button.role), std::move(button.on_press));
    return true;
}

inline bool Builder::visitSwitcher(ADescription& description)
{
    auto& switcher = static_cast<SwitcherDescription&>(description);
    result_ = std::make_unique<SwitcherObject>(context_, placement_.value_or(switcher.box), std::move(switcher.text),
                                               loadIcon(switcher.icon), resolveRole(switcher.role),
                                               std::move(switcher.on_active), std::move(switcher.on_deactive));
    return true;
}

inline bool Builder::visitContextMenu(ADescription& description)
{
    auto&             menu   = static_cast<ContextMenuDescription&>(description);
    const std::size_t count  = menu.objects.size();
    const BoxF        bounds = placement_.value_or(makeBox(menu.min_point, menuSize(menu.direction, count)));

    auto object = std::make_unique<ContextMenuDescriptionObject>(context_, bounds, resolveRole(menu.role),
                                                                 std::move(menu.on_event));
    if (!addChildren(*object, menu.objects, slotsFor(bounds.size(), menu.direction, count)))
    {
        return false;
    }
    result_ = std::move(object);
    return true;
}

inline bool Builder::visitPoolMenu(ADescription& description)
{
    auto&      pool   = static_cast<PoolMenuDescription&>(description);
    const BoxF bounds = placement_.value_or(makeBox(pool.min_point, pool.size));

    auto object = std::make_unique<PoolMenuDescriptionObject>(context_, bounds, resolveRole(pool.role));
    if (!addChildren(*object, pool.objects, slotsFor(bounds.size(), pool.direction, pool.objects.size())))
    {
        return false;
    }
    result_ = std::move(object);
    return true;
}

inline bool Builder::visitWindow(ADescription& description)
{
    auto&      window = static_cast<WindowDescription&>(description);
    const BoxF bounds = placement_.value_or(window.box);
    const Role role   = resolveRole(window.role);

    auto object = std::make_unique<WindowDescriptionObject>(context_, bounds, std::move(window.label), role,
                                                            std::move(window.on_event));

    if (window.on_close)
    {
        const Vec2F size = bounds.size();
        const Vec2F close_pos(size.x() - CLOSE_BUTTON_SIZE - PADDING, PADDING);
        object->addChild(std::make_unique<ButtonObject>(context_,
                                                        makeBox(close_pos, Vec2F(CLOSE_BUTTON_SIZE, CLOSE_BUTTON_SIZE)),
                                                        std::optional<std::string>("X"), std::nullopt, role,
                                                        std::move(*window.on_close)));
    }

    if (!addChildren(*object, window.objects, std::vector<Placement>(window.objects.size())))
    {
        return false;
    }
    result_ = std::move(object);
    return true;
}

inline bool Builder::addChildren(AObject& parent, std::vector<std::unique_ptr<ADescription>>& objects,
                                 const std::vector<Placement>& placements)
{
    for (std::size_t i = 0; i < objects.size(); ++i)
    {
        Builder child(context_, placements[i]);
        if (!objects[i]->accept(child))
        {
            return false;
        }
        parent.addChild(child.take());
    }
    return true;
}

} // namespace

inline UIWi::UIWi()
{
    registryTheme(DEFAULT_THEME_NAME, Color{53, 132, 228, OPAQUE_ALPHA}, Color{222, 221, 218, OPAQUE_ALPHA},
                  Color{246, 245, 244, OPAQUE_ALPHA}, Color{28, 113, 216, OPAQUE_ALPHA},
                  Color{224, 27, 36, OPAQUE_ALPHA});
    registryTheme(DARK_THEME_NAME, Color{53, 132, 228, OPAQUE_ALPHA}, Color{61, 61, 61, OPAQUE_ALPHA},
                  Color{36, 36, 36, OPAQUE_ALPHA}, Color{98, 160, 234, OPAQUE_ALPHA},
                  Color{255, 123, 107, OPAQUE_ALPHA});
    setTheme(DEFAULT_THEME_NAME);
}

inline std::unique_ptr<AObject> UIWi::create(ADescription& description)
{
    Builder builder(context_, std::nullopt);
    if (!description.accept(builder))
    {
        return nullptr;
    }
    return builder.take();
}

inline void UIWi::registryTheme(std::string name, const Color& primary, const Color& secondary, const Color& neutral,
                                const Color& accent, const Color& unical)
{
    themes_.insert_or_assign(std::move(name), makeTheme(primary, secondary, neutral, accent, unical));
}

inline bool UIWi::setTheme(std::string name)
{
    const auto it = themes_.find(name);
    if (it == themes_.end())
    {
        return false;
    }
    context_.theme = &it->second;
    return true;
}

inline bool UIWi::setFont(std::string path)
{
    IRenderer* renderer = queryRenderer();
    if (!renderer)
    {
        return false;
    }
    context_.font = renderer->loadFont(path.c_str());
    return true;
}

} // namespace UI

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    static UI::UIWi framework;

    return ApplicationCore::instance().registerInterface(
               IID_IUIFramework, reinterpret_cast<void*>(static_cast<UI::IFramework*>(&framework))
           ) == ApplicationCore::Error::SUCCESS;
}
