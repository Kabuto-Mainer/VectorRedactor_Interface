#include "Core/ApplicationCore.hpp"
#include "IUIFramework.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/Package.hpp"
#include "Utils/ACommand.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/TObject.hpp"
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
    IRenderer::Color base;
    IRenderer::Color hover;
    IRenderer::Color pressed;
    IRenderer::Color text;
    IRenderer::Color border;
    IRenderer::Color surface;
    IRenderer::Color surface_text;
    IRenderer::Color canvas;
};

IRenderer::Color mix(const IRenderer::Color& from, const IRenderer::Color& to, float ratio)
{
    const auto channel = [ratio](uint8_t from_value, uint8_t to_value)
    {
        return static_cast<uint8_t>(static_cast<float>(from_value) +
                                    (static_cast<float>(to_value) - static_cast<float>(from_value)) * ratio);
    };
    return IRenderer::Color{channel(from.r, to.r), channel(from.g, to.g), channel(from.b, to.b), OPAQUE_ALPHA};
}

float luminance(const IRenderer::Color& color)
{
    return (0.2126F * color.r + 0.7152F * color.g + 0.0722F * color.b) / CHANNEL_MAX;
}

IRenderer::Color contrastOf(const IRenderer::Color& color)
{
    if (luminance(color) < LIGHTNESS_THRESHOLD)
    {
        return IRenderer::Color{255, 255, 255, OPAQUE_ALPHA};
    }
    return IRenderer::Color{0, 0, 0, OPAQUE_ALPHA};
}

Palette derive(const IRenderer::Color& base, const IRenderer::Color& surface, const IRenderer::Color& canvas)
{
    const IRenderer::Color text   = contrastOf(base);
    const IRenderer::Color panel  = mix(surface, base, SECONDARY_TINT);
    const IRenderer::Color tinted = mix(canvas, base, CANVAS_TINT);
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
    IRenderer::Color                                                  canvas;
    IRenderer::Color                                                  surface;
    IRenderer::Color                                                  accent;
    std::array<IRenderer::Color, static_cast<std::size_t>(IUIFramework::Role::MAX)> role_bases;

    Palette palette(IUIFramework::IUIFramework::Role role) const;
};

inline Palette Theme::palette(IUIFramework::Role role) const
{
    return derive(role_bases[static_cast<std::size_t>(role)], surface, canvas);
}

Theme makeTheme(const IRenderer::Color& primary, const IRenderer::Color& secondary, const IRenderer::Color& neutral,
                const IRenderer::Color& accent, const IRenderer::Color& unical)
{
    const bool             dark = luminance(neutral) < LIGHTNESS_THRESHOLD;
    const IRenderer::Color white{255, 255, 255, OPAQUE_ALPHA};
    const IRenderer::Color surface = mix(neutral, white, dark ? DARK_SURFACE_LIFT : LIGHT_SURFACE_LIFT);
    const IRenderer::Color button  = mix(neutral, secondary, SECONDARY_TINT);

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

class UIWi final : public IUIFramework
{
public:
    UIWi();

    UIWi(const UIWi&)            = delete;
    UIWi& operator=(const UIWi&) = delete;

    std::unique_ptr<Button>       create(ButtonDescription& description) override;
    std::unique_ptr<Switcher>     create(SwitcherDescription& description) override;
    std::unique_ptr<ContextMenu>  create(ContextMenuDescription& description) override;
    std::unique_ptr<ButtonPool>   create(ButtonPoolDescription& description) override;
    std::unique_ptr<SwitcherPool> create(SwitcherPoolDescription& description) override;
    std::unique_ptr<WindowMenu>   create(WindowMenuDescription& description) override;

    void registryTheme(const std::string& name, const IRenderer::Color& primary, const IRenderer::Color& secondary,
                       const IRenderer::Color& neutral, const IRenderer::Color& accent,
                       const IRenderer::Color& unical) override;

    bool setTheme(const std::string& name) override;

    bool setFont(const std::string& path) override;

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

const IInputPump::EventMouse* asMouse(const IInputPump::Event& event)
{
    return std::get_if<IInputPump::EventMouse>(&event);
}

std::optional<IRenderer::TextureID> loadIcon(const std::optional<std::string>& path)
{
    IRenderer* renderer = queryRenderer();
    if (!path || !renderer)
    {
        return std::nullopt;
    }
    return renderer->loadTexture(*path);
}

void drawPanel(IRenderer* renderer, const IRenderer::Color& fill, const IRenderer::Color& border, const BoxF& box)
{
    renderer->renderRect(fill, box, NO_ANGLE);
    renderer->renderRectLines(border, box, NO_ANGLE);
}

void drawText(IRenderer* renderer, const RenderContext& context, const std::string& text, const IRenderer::Color& color,
              const BoxF& box)
{
    if (!context.font)
    {
        return;
    }
    renderer->renderText(color, box, text, IRenderer::TextFormat{.font = *context.font, .font_size = FONT_SIZE});
}

void drawContent(IRenderer* renderer, const RenderContext& context, const std::optional<IRenderer::TextureID>& icon,
                 const std::optional<std::string>& text, const IRenderer::Color& color, const BoxF& box)
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

Vec2F menuSize(IUIFramework::Direction direction, std::size_t count)
{
    const auto items = static_cast<float>(count);
    return direction == IUIFramework::Direction::VERTICAL ? Vec2F(MENU_ITEM_WIDTH, MENU_ITEM_HEIGHT * items) :
                                              Vec2F(MENU_ITEM_WIDTH * items, MENU_ITEM_HEIGHT);
}

typedef std::optional<BoxF> Placement;

std::vector<Placement> slotsFor(const Vec2F& size, IUIFramework::Direction direction, std::size_t count)
{
    std::vector<Placement> slots;
    if (count == 0)
    {
        return slots;
    }

    const auto  divider = static_cast<float>(count);
    const Vec2F slot =
        direction == IUIFramework::Direction::VERTICAL ? Vec2F(size.x(), size.y() / divider) : Vec2F(size.x() / divider, size.y());
    const Vec2F step = direction == IUIFramework::Direction::VERTICAL ? Vec2F(0.0F, slot.y()) : Vec2F(slot.x(), 0.0F);

    Vec2F position;
    for (std::size_t i = 0; i < count; ++i)
    {
        slots.emplace_back(makeBox(position, slot));
        position = position + step;
    }
    return slots;
}

void layoutChildren(TObject& parent, const Vec2F& size, IUIFramework::Direction direction)
{
    const std::vector<Placement> placements = slotsFor(size, direction, parent.children().size());
    std::size_t                  index      = 0;
    for (auto& child : parent.children())
    {
        static_cast<TWidget&>(*child).setBox(*placements[index++]);
    }
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
    PressResult process(const BoxF& bounds, const IInputPump::Event& event);

    IRenderer::Color background(const IRenderer::Color& idle, const Palette& palette) const;

private:
    bool hovered_ = false;
    bool pressed_ = false;
};

inline PressResult PressTracker::process(const BoxF& bounds, const IInputPump::Event& event)
{
    const IInputPump::EventMouse* mouse = asMouse(event);
    if (!mouse)
    {
        return PressResult::IGNORED;
    }

    const bool inside = bounds.contains(mouse->cursorPos());
    hovered_          = inside;

    switch (mouse->type())
    {
        case IInputPump::EventMouse::Type::LEFT_BUTTON_PRESSING:
            if (inside)
            {
                pressed_ = true;
                return PressResult::CONSUMED;
            }
            return PressResult::IGNORED;

        case IInputPump::EventMouse::Type::LEFT_BUTTON_RELEASING:
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

inline IRenderer::Color PressTracker::background(const IRenderer::Color& idle, const Palette& palette) const
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

void runAction(IUIFramework::Action& action)
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

Vec2F globalCenter(const TWidget& widget)
{
    return widget.mapLocalToGlobal(widget.box().size() / 2.0F);
}

BoxF globalBounds(const TWidget& widget)
{
    BoxF bounds(widget.box().size());
    bounds.setCenter(globalCenter(widget));
    return bounds;
}

class ButtonObject final : public IUIFramework::Button
{
public:
    ButtonObject(const RenderContext& context, const BoxF& bounds, std::optional<std::string> text,
                 std::optional<IRenderer::TextureID> icon, IUIFramework::Role role, IUIFramework::Action on_press);

    bool processEvent(const IInputPump::Event& event) override;

    void render() override;

private:
    const RenderContext&                context_;
    std::optional<std::string>          text_;
    std::optional<IRenderer::TextureID> icon_;
    IUIFramework::Role                                role_;
    IUIFramework::Action                  on_press_;
    PressTracker                        press_;
};

inline ButtonObject::ButtonObject(const RenderContext& context, const BoxF& bounds, std::optional<std::string> text,
                                  std::optional<IRenderer::TextureID> icon, IUIFramework::Role role, IUIFramework::Action on_press) :
    context_(context), text_(std::move(text)), icon_(icon), role_(role), on_press_(std::move(on_press))
{
    setBox(bounds);
}

inline bool ButtonObject::processEvent(const IInputPump::Event& event)
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

class SwitcherObject final : public IUIFramework::Switcher
{
public:
    SwitcherObject(const RenderContext& context, const BoxF& bounds, std::optional<std::string> text,
                   std::optional<IRenderer::TextureID> icon, IUIFramework::Role role, IUIFramework::Action on_active,
                   IUIFramework::Action on_deactive);

    bool processEvent(const IInputPump::Event& event) override;

    void render() override;

    bool isActive() const;
    void activate();
    void deactivate();

private:
    const RenderContext&                context_;
    std::optional<std::string>          text_;
    std::optional<IRenderer::TextureID> icon_;
    IUIFramework::Role                                role_;
    IUIFramework::Action                  on_active_;
    IUIFramework::Action                  on_deactive_;
    PressTracker                        press_;
    bool                                active_ = false;
};

class SwitcherPoolObject final : public IUIFramework::SwitcherPool
{
public:
    SwitcherPoolObject(const RenderContext& context, const Vec2F& min_point, const Vec2F& size, IUIFramework::Direction direction,
                       IUIFramework::Role role);

    void add(std::unique_ptr<IUIFramework::Switcher> switcher) override;

    bool processEvent(const IInputPump::Event& event) override;

    void render() override;

    void select(SwitcherObject& target);

private:
    const RenderContext& context_;
    Vec2F                size_;
    IUIFramework::Direction            direction_;
    IUIFramework::Role                 role_;
};

inline SwitcherObject::SwitcherObject(const RenderContext& context, const BoxF& bounds, std::optional<std::string> text,
                                      std::optional<IRenderer::TextureID> icon, IUIFramework::Role role, IUIFramework::Action on_active,
                                      IUIFramework::Action on_deactive) :
    context_(context), text_(std::move(text)), icon_(icon), role_(role), on_active_(std::move(on_active)),
    on_deactive_(std::move(on_deactive))
{
    setBox(bounds);
}

inline bool SwitcherObject::processEvent(const IInputPump::Event& event)
{
    switch (press_.process(globalBounds(*this), event))
    {
        case PressResult::CLICKED:
            if (auto* pool = dynamic_cast<SwitcherPoolObject*>(parent()))
            {
                pool->select(*this);
            }
            else if (active_)
            {
                deactivate();
            }
            else
            {
                activate();
            }
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

    const Palette          palette = context_.theme->palette(role_);
    const BoxF             bounds  = globalBounds(*this);
    const IRenderer::Color idle    = active_ ? palette.base : palette.surface;
    const IRenderer::Color text    = active_ ? palette.text : palette.surface_text;
    const IRenderer::Color border  = active_ ? context_.theme->accent : palette.border;
    drawPanel(renderer, press_.background(idle, palette), border, bounds);
    drawContent(renderer, context_, icon_, text_, text, bounds);
}

inline bool SwitcherObject::isActive() const
{
    return active_;
}

inline void SwitcherObject::activate()
{
    active_ = true;
    runAction(on_active_);
}

inline void SwitcherObject::deactivate()
{
    active_ = false;
    runAction(on_deactive_);
}

inline SwitcherPoolObject::SwitcherPoolObject(const RenderContext& context, const Vec2F& min_point, const Vec2F& size,
                                              IUIFramework::Direction direction, IUIFramework::Role role) :
    context_(context), size_(size), direction_(direction), role_(role)
{
    setBox(makeBox(min_point, size));
}

inline void SwitcherPoolObject::add(std::unique_ptr<IUIFramework::Switcher> switcher)
{
    addChild(std::move(switcher));
    layoutChildren(*this, size_, direction_);
}

inline bool SwitcherPoolObject::processEvent(const IInputPump::Event& event)
{
    return processEventChildren(event);
}

inline void SwitcherPoolObject::render()
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

inline void SwitcherPoolObject::select(SwitcherObject& target)
{
    if (target.isActive())
    {
        return;
    }

    for (auto& child : children())
    {
        auto& switcher = static_cast<SwitcherObject&>(*child);
        if (switcher.isActive())
        {
            switcher.deactivate();
        }
    }
    target.activate();
}

class ButtonPoolObject final : public IUIFramework::ButtonPool
{
public:
    ButtonPoolObject(const RenderContext& context, const Vec2F& min_point, const Vec2F& size, IUIFramework::Direction direction,
                     IUIFramework::Role role);

    void add(std::unique_ptr<IUIFramework::Button> button) override;

    bool processEvent(const IInputPump::Event& event) override;

    void render() override;

private:
    const RenderContext& context_;
    Vec2F                size_;
    IUIFramework::Direction            direction_;
    IUIFramework::Role                 role_;
};

inline ButtonPoolObject::ButtonPoolObject(const RenderContext& context, const Vec2F& min_point, const Vec2F& size,
                                          IUIFramework::Direction direction, IUIFramework::Role role) :
    context_(context), size_(size), direction_(direction), role_(role)
{
    setBox(makeBox(min_point, size));
}

inline void ButtonPoolObject::add(std::unique_ptr<IUIFramework::Button> button)
{
    addChild(std::move(button));
    layoutChildren(*this, size_, direction_);
}

inline bool ButtonPoolObject::processEvent(const IInputPump::Event& event)
{
    return processEventChildren(event);
}

inline void ButtonPoolObject::render()
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

class ContextMenuObject final : public IUIFramework::ContextMenu
{
public:
    ContextMenuObject(const RenderContext& context, const Vec2F& min_point, IUIFramework::Direction direction, IUIFramework::Role role,
                      std::optional<IUIFramework::EventHandler> on_event);

    void add(std::unique_ptr<IUIFramework::Button> button) override;
    void add(std::unique_ptr<IUIFramework::Switcher> switcher) override;

    bool processEvent(const IInputPump::Event& event) override;

    void render() override;

private:
    void relayout();

    bool isPressOutside(const IInputPump::Event& event) const;

    const RenderContext&        context_;
    Vec2F                       min_point_;
    IUIFramework::Direction                   direction_;
    IUIFramework::Role                        role_;
    std::optional<IUIFramework::EventHandler> on_event_;
    bool                        visible_ = true;
};

inline ContextMenuObject::ContextMenuObject(const RenderContext& context, const Vec2F& min_point, IUIFramework::Direction direction,
                                            IUIFramework::Role role, std::optional<IUIFramework::EventHandler> on_event) :
    context_(context), min_point_(min_point), direction_(direction), role_(role), on_event_(std::move(on_event))
{
    relayout();
}

inline void ContextMenuObject::add(std::unique_ptr<IUIFramework::Button> button)
{
    addChild(std::move(button));
    relayout();
}

inline void ContextMenuObject::add(std::unique_ptr<IUIFramework::Switcher> switcher)
{
    addChild(std::move(switcher));
    relayout();
}

inline void ContextMenuObject::relayout()
{
    setBox(makeBox(min_point_, menuSize(direction_, children().size())));
    layoutChildren(*this, box().size(), direction_);
}

inline bool ContextMenuObject::processEvent(const IInputPump::Event& event)
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

inline void ContextMenuObject::render()
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

inline bool ContextMenuObject::isPressOutside(const IInputPump::Event& event) const
{
    const IInputPump::EventMouse* mouse = asMouse(event);
    return mouse &&
           mouse->type() == IInputPump::EventMouse::Type::LEFT_BUTTON_PRESSING &&
           !globalBounds(*this).contains(mouse->cursorPos());
}

class WindowMenuObject final : public IUIFramework::WindowMenu
{
public:
    WindowMenuObject(const RenderContext& context, const BoxF& bounds, std::optional<std::string> label, IUIFramework::Role role,
                     std::optional<IUIFramework::EventHandler> on_event, std::optional<IUIFramework::Action> on_close);

    void add(std::unique_ptr<IUIFramework::Button> button) override;
    void add(std::unique_ptr<IUIFramework::Switcher> switcher) override;
    void add(std::unique_ptr<IUIFramework::ContextMenu> menu) override;
    void add(std::unique_ptr<IUIFramework::ButtonPool> pool) override;
    void add(std::unique_ptr<IUIFramework::SwitcherPool> pool) override;

    bool processEvent(const IInputPump::Event& event) override;

    void render() override;

private:
    const RenderContext&        context_;
    std::optional<std::string>  label_;
    IUIFramework::Role                        role_;
    std::optional<IUIFramework::EventHandler> on_event_;
};

inline WindowMenuObject::WindowMenuObject(const RenderContext& context, const BoxF& bounds,
                                          std::optional<std::string> label, IUIFramework::Role role,
                                          std::optional<IUIFramework::EventHandler> on_event,
                                          std::optional<IUIFramework::Action> on_close) :
    context_(context), label_(std::move(label)), role_(role), on_event_(std::move(on_event))
{
    setBox(bounds);

    if (on_close)
    {
        const Vec2F size = bounds.size();
        const Vec2F close_pos(size.x() - CLOSE_BUTTON_SIZE - PADDING, PADDING);
        addChild(std::make_unique<ButtonObject>(context_,
                                                makeBox(close_pos, Vec2F(CLOSE_BUTTON_SIZE, CLOSE_BUTTON_SIZE)),
                                                std::optional<std::string>("X"), std::nullopt, role_,
                                                std::move(*on_close)));
    }
}

inline void WindowMenuObject::add(std::unique_ptr<IUIFramework::Button> button)
{
    addChild(std::move(button));
}

inline void WindowMenuObject::add(std::unique_ptr<IUIFramework::Switcher> switcher)
{
    addChild(std::move(switcher));
}

inline void WindowMenuObject::add(std::unique_ptr<IUIFramework::ContextMenu> menu)
{
    addChild(std::move(menu));
}

inline void WindowMenuObject::add(std::unique_ptr<IUIFramework::ButtonPool> pool)
{
    addChild(std::move(pool));
}

inline void WindowMenuObject::add(std::unique_ptr<IUIFramework::SwitcherPool> pool)
{
    addChild(std::move(pool));
}

inline bool WindowMenuObject::processEvent(const IInputPump::Event& event)
{
    if (on_event_ && (*on_event_)(event))
    {
        return true;
    }
    return processEventChildren(event);
}

inline void WindowMenuObject::render()
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

} // namespace

inline UIWi::UIWi()
{
    registryTheme(DEFAULT_THEME_NAME, IRenderer::Color{53, 132, 228, OPAQUE_ALPHA},
                  IRenderer::Color{222, 221, 218, OPAQUE_ALPHA}, IRenderer::Color{246, 245, 244, OPAQUE_ALPHA},
                  IRenderer::Color{28, 113, 216, OPAQUE_ALPHA}, IRenderer::Color{224, 27, 36, OPAQUE_ALPHA});
    registryTheme(DARK_THEME_NAME, IRenderer::Color{53, 132, 228, OPAQUE_ALPHA},
                  IRenderer::Color{61, 61, 61, OPAQUE_ALPHA}, IRenderer::Color{36, 36, 36, OPAQUE_ALPHA},
                  IRenderer::Color{98, 160, 234, OPAQUE_ALPHA}, IRenderer::Color{255, 123, 107, OPAQUE_ALPHA});
    setTheme(DEFAULT_THEME_NAME);
}

inline std::unique_ptr<IUIFramework::Button> UIWi::create(ButtonDescription& description)
{
    return std::make_unique<ButtonObject>(context_, description.box, std::move(description.text),
                                          loadIcon(description.icon), description.role,
                                          std::move(description.on_press));
}

inline std::unique_ptr<IUIFramework::Switcher> UIWi::create(SwitcherDescription& description)
{
    return std::make_unique<SwitcherObject>(context_, description.box, std::move(description.text),
                                            loadIcon(description.icon), description.role,
                                            std::move(description.on_active), std::move(description.on_deactive));
}

inline std::unique_ptr<IUIFramework::ContextMenu> UIWi::create(ContextMenuDescription& description)
{
    return std::make_unique<ContextMenuObject>(context_, description.min_point, description.direction,
                                               description.role, std::move(description.on_event));
}

inline std::unique_ptr<IUIFramework::ButtonPool> UIWi::create(ButtonPoolDescription& description)
{
    return std::make_unique<ButtonPoolObject>(context_, description.min_point, description.size, description.direction,
                                              description.role);
}

inline std::unique_ptr<IUIFramework::SwitcherPool> UIWi::create(SwitcherPoolDescription& description)
{
    return std::make_unique<SwitcherPoolObject>(context_, description.min_point, description.size,
                                                description.direction, description.role);
}

inline std::unique_ptr<IUIFramework::WindowMenu> UIWi::create(WindowMenuDescription& description)
{
    return std::make_unique<WindowMenuObject>(context_, description.box, std::move(description.label),
                                              description.role, std::move(description.on_event),
                                              std::move(description.on_close));
}

inline void UIWi::registryTheme(const std::string& name, const IRenderer::Color& primary,
                                const IRenderer::Color& secondary, const IRenderer::Color& neutral,
                                const IRenderer::Color& accent, const IRenderer::Color& unical)
{
    themes_.insert_or_assign(std::move(name), makeTheme(primary, secondary, neutral, accent, unical));
}

inline bool UIWi::setTheme(const std::string& name)
{
    const auto it = themes_.find(name);
    if (it == themes_.end())
    {
        return false;
    }
    context_.theme = &it->second;
    return true;
}

inline bool UIWi::setFont(const std::string& path)
{
    IRenderer* renderer = queryRenderer();
    if (!renderer)
    {
        return false;
    }
    context_.font = renderer->loadFont(path);
    return true;
}

} // namespace UI

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    static UI::UIWi framework;

    return ApplicationCore::instance().registerInterface(
               IID_IUIFramework, reinterpret_cast<void*>(static_cast<IUIFramework*>(&framework))
           ) == ApplicationCore::Error::SUCCESS;
}
