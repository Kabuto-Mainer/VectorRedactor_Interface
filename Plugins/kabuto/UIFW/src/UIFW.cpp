#include "Core/ApplicationCore.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/Package.hpp"
#include "Utils/ACommand.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/TObject.hpp"
#include "Utils/Vec2F.hpp"
#include "export/IUIFramework.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace Kabuto
{

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
constexpr float   MENU_ITEM_WIDTH      = 120.0F;
constexpr float   MENU_ITEM_HEIGHT     = 28.0F;
constexpr float   POOL_GAP             = 4.0F;
constexpr float   POOL_DRAG_EDGE       = 2.0F;
constexpr float   TEXT_WIDTH_FACTOR    = 0.6F;
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
    IRenderer::Color                                                       canvas;
    IRenderer::Color                                                       surface;
    IRenderer::Color                                                       accent;
    std::array<Palette, static_cast<std::size_t>(IUIFramework::Role::MAX)> palettes;

    Palette palette(IUIFramework::Role role) const;
};

inline Palette Theme::palette(IUIFramework::Role role) const
{
    return palettes[static_cast<std::size_t>(role)];
}

Theme makeTheme(const IRenderer::Color& primary, const IRenderer::Color& secondary, const IRenderer::Color& neutral,
                const IRenderer::Color& accent, const IRenderer::Color& unical)
{
    const bool             dark = luminance(neutral) < LIGHTNESS_THRESHOLD;
    const IRenderer::Color white{255, 255, 255, OPAQUE_ALPHA};
    const IRenderer::Color surface = mix(neutral, white, dark ? DARK_SURFACE_LIFT : LIGHT_SURFACE_LIFT);
    const IRenderer::Color button  = mix(neutral, secondary, SECONDARY_TINT);

    const std::array<IRenderer::Color, static_cast<std::size_t>(IUIFramework::Role::MAX)> role_bases{button, primary,
                                                                                                     unical};

    std::array<Palette, static_cast<std::size_t>(IUIFramework::Role::MAX)> palettes{};
    for (std::size_t index = 0; index < role_bases.size(); ++index)
    {
        palettes[index] = derive(role_bases[index], surface, neutral);
    }

    return Theme{neutral, surface, accent, palettes};
}

struct RenderContext
{
    const Theme*                     theme = nullptr;
    std::optional<IRenderer::FontID> font;
    mutable IRenderer*               renderer = nullptr;
};

BoxF makeBox(const Vec2F& pos, const Vec2F& size)
{
    return BoxF(pos, pos + size);
}

struct TextMetrics
{
    float width;
    float height;
};

TextMetrics measureText(const std::string& text)
{
    std::size_t max_line = 0;
    std::size_t line     = 0;
    std::size_t lines    = 1;

    for (const unsigned char character : text)
    {
        if (character == '\n')
        {
            max_line = std::max(max_line, line);
            line     = 0;
            ++lines;
            continue;
        }
        /* Count UTF-8 code points, not continuation bytes. */
        if ((character & 0xC0U) != 0x80U)
        {
            ++line;
        }
    }
    max_line = std::max(max_line, line);

    return TextMetrics{static_cast<float>(max_line) * static_cast<float>(FONT_SIZE) * TEXT_WIDTH_FACTOR,
                       static_cast<float>(lines) * static_cast<float>(FONT_SIZE)};
}

BoxF ensureMinSize(const BoxF& box, float min_width, float min_height)
{
    const Vec2F size   = box.size();
    const Vec2F origin = box.topLeft();
    return makeBox(origin, Vec2F(std::max(size.x(), min_width), std::max(size.y(), min_height)));
}

BoxF fitToText(const BoxF& box, const std::optional<std::string>& text, bool has_icon)
{
    if (!text || text->empty())
    {
        return box;
    }

    const TextMetrics metrics    = measureText(*text);
    const float       icon_width = has_icon ? static_cast<float>(FONT_SIZE) + PADDING : 0.0F;
    const float       min_width  = metrics.width + 2.0F * PADDING + icon_width;
    const float       min_height = metrics.height + 2.0F * PADDING;
    return ensureMinSize(box, min_width, min_height);
}

BoxF fitWindowBox(const BoxF& box, const std::optional<std::string>& label, float close_width)
{
    const TextMetrics metrics   = (label && !label->empty()) ? measureText(*label) : TextMetrics{0.0F, 0.0F};
    const float       reserved  = close_width > 0.0F ? close_width + 2.0F * PADDING : 0.0F;
    const float       min_width = metrics.width + 2.0F * PADDING + reserved;
    return ensureMinSize(box, min_width, HEADER_HEIGHT);
}

} // namespace

class UIWi final : public IUIFramework
{
public:
    UIWi(const UIWi&)            = delete;
    UIWi& operator=(const UIWi&) = delete;

    static UIWi& instance();

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
    UIWi();
    ~UIWi() override = default;

    void loadFonts();

    RenderContext                          context_;
    std::unordered_map<std::string, Theme> themes_;
};

namespace
{

constexpr char FONT_LIST_ENV_NAME[] = "GDINTERFACE_UIFW_ASSETS";
constexpr char FONT_LIST_FILENAME[] = "font_list.txt";
constexpr char ASSETS_DIRECTORY[]   = "assets";
constexpr char TRIM_CHARACTERS[]    = " \t\r\n";

std::optional<std::filesystem::path> moduleDirectory(const void* symbol)
{
    Dl_info info{};
    if (dladdr(symbol, &info) == 0 || info.dli_fname == nullptr)
    {
        return std::nullopt;
    }

    std::error_code             error;
    const std::filesystem::path module = std::filesystem::absolute(info.dli_fname, error);
    if (error)
    {
        return std::nullopt;
    }
    return module.parent_path();
}

std::optional<std::filesystem::path> fontListPath()
{
    if (const char* configured = std::getenv(FONT_LIST_ENV_NAME); configured != nullptr && configured[0] != '\0')
    {
        std::error_code             error;
        const std::filesystem::path path = configured;
        if (std::filesystem::is_directory(path, error))
        {
            return path / FONT_LIST_FILENAME;
        }
        return path;
    }

    const std::optional<std::filesystem::path> directory =
        moduleDirectory(reinterpret_cast<const void*>(&fontListPath));
    if (!directory)
    {
        return std::nullopt;
    }
    return *directory / ASSETS_DIRECTORY / FONT_LIST_FILENAME;
}

std::vector<std::string> readFontList(const std::filesystem::path& path)
{
    std::vector<std::string> fonts;
    std::ifstream            list(path);
    if (!list.is_open())
    {
        return fonts;
    }

    std::string line;
    while (std::getline(list, line))
    {
        const std::size_t first = line.find_first_not_of(TRIM_CHARACTERS);
        if (first == std::string::npos)
        {
            continue;
        }
        const std::size_t last = line.find_last_not_of(TRIM_CHARACTERS);
        std::string       font = line.substr(first, last - first + 1);
        if (font.front() == '#')
        {
            continue;
        }
        fonts.push_back(std::move(font));
    }
    return fonts;
}

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

IRenderer* acquireRenderer(const RenderContext& context)
{
    if (context.renderer == nullptr)
    {
        context.renderer = queryRenderer();
    }
    return context.renderer;
}

const IInputPump::EventMouse* asMouse(const IInputPump::Event& event)
{
    return std::get_if<IInputPump::EventMouse>(&event);
}

std::optional<IRenderer::TextureID> loadIcon(const std::optional<IUIFramework::Icon>& icon)
{
    if (!icon)
    {
        return std::nullopt;
    }

    if (const auto* texture = std::get_if<IRenderer::TextureID>(&*icon))
    {
        return *texture;
    }

    IRenderer* renderer = queryRenderer();
    if (!renderer)
    {
        return std::nullopt;
    }
    return renderer->loadTexture(std::get<std::string>(*icon));
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
    renderer->renderText(color, box, text,
                         IRenderer::TextFormat{.font            = *context.font,
                                               .font_size       = FONT_SIZE,
                                               .overflow_action = IRenderer::TextOverflowAction::TAILREPLACE,
                                               .h_align         = IRenderer::TextHAlign::LEFT,
                                               .w_align         = IRenderer::TextWAlign::CENTER,
                                               .padding         = PADDING,
                                               .line_spacing    = 0.0F,
                                               .wrap            = false});
}

BoxF insetBox(const BoxF& box, float inset)
{
    return makeBox(box.topLeft() + Vec2F(inset, inset), box.size() - Vec2F(2.0F * inset));
}

BoxF squareBoxCentered(const BoxF& box)
{
    const float side = std::min(box.size().x(), box.size().y());
    BoxF        square(Vec2F(side, side));
    square.setCenter(box.center());
    return square;
}

BoxF leftSquareBox(const BoxF& box)
{
    const float side = std::min(box.size().x(), box.size().y());
    const float top  = box.center().y() - side / 2.0F;
    return makeBox(Vec2F(box.left(), top), Vec2F(side, side));
}

float contentPadding(const BoxF& box)
{
    const float side = std::min(box.size().x(), box.size().y());
    return std::min(PADDING, std::max(0.0F, (side - static_cast<float>(FONT_SIZE) - PADDING) / 2.0F));
}

void drawControlText(IRenderer* renderer, const RenderContext& context, const std::string& text,
                     const IRenderer::Color& color, const BoxF& box)
{
    if (!context.font)
    {
        return;
    }
    renderer->renderText(color, box, text,
                         IRenderer::TextFormat{.font            = *context.font,
                                               .font_size       = FONT_SIZE,
                                               .overflow_action = IRenderer::TextOverflowAction::TAILREPLACE,
                                               .h_align         = IRenderer::TextHAlign::CENTER,
                                               .w_align         = IRenderer::TextWAlign::CENTER,
                                               .padding         = contentPadding(box),
                                               .line_spacing    = 0.0F,
                                               .wrap            = false});
}

void drawControlContent(IRenderer* renderer, const RenderContext& context,
                        const std::optional<IRenderer::TextureID>& icon, const std::optional<std::string>& text,
                        const IRenderer::Color& color, const BoxF& box)
{
    const BoxF inner = insetBox(box, contentPadding(box));

    if (icon && text)
    {
        const BoxF icon_box = leftSquareBox(inner);
        renderer->renderTexture(*icon, icon_box, NO_ANGLE);

        const BoxF text_box =
            makeBox(Vec2F(icon_box.right(), box.top()), Vec2F(box.right() - icon_box.right(), box.size().y()));
        drawControlText(renderer, context, *text, color, text_box);
        return;
    }

    if (icon)
    {
        renderer->renderTexture(*icon, squareBoxCentered(inner), NO_ANGLE);
        return;
    }

    if (text)
    {
        drawControlText(renderer, context, *text, color, box);
    }
}

BoxF layoutPool(std::list<std::unique_ptr<TObject>>& children, const BoxF& bounds, IUIFramework::Direction direction)
{
    /* Children keep their current main-axis size and are stretched across the
     * pool. When they do not fit, the pool grows forward along its direction. */
    const bool  vertical = direction == IUIFramework::Direction::VERTICAL;
    const Vec2F origin   = bounds.topLeft();
    const Vec2F current  = bounds.size();

    std::vector<float> main_sizes;
    main_sizes.reserve(children.size());

    float total     = 0.0F;
    float max_cross = 0.0F;
    for (const auto& child : children)
    {
        const Vec2F child_size  = static_cast<const TWidget&>(*child).box().size();
        const float main        = std::max(vertical ? child_size.y() : child_size.x(), 0.0F);
        const float child_cross = vertical ? child_size.x() : child_size.y();
        main_sizes.push_back(main);
        total += main;
        max_cross = std::max(max_cross, child_cross);
    }

    if (!main_sizes.empty())
    {
        total += POOL_GAP * static_cast<float>(main_sizes.size() - 1);
    }

    const float cross = std::max(vertical ? current.x() : current.y(), max_cross);
    Vec2F       size  = current;
    if (vertical)
    {
        size.setX(cross);
        size.setY(std::max(current.y(), total));
    }
    else
    {
        size.setY(cross);
        size.setX(std::max(current.x(), total));
    }

    float       offset = 0.0F;
    std::size_t index  = 0;
    for (auto& child : children)
    {
        const float main       = main_sizes[index++];
        const Vec2F position   = vertical ? Vec2F(0.0F, offset) : Vec2F(offset, 0.0F);
        const Vec2F child_size = vertical ? Vec2F(cross, main) : Vec2F(main, cross);
        static_cast<TWidget&>(*child).setBox(makeBox(position, child_size));
        offset += main + POOL_GAP;
    }

    return makeBox(origin, size);
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
    PressTracker()                               = default;
    PressTracker(const PressTracker&)            = delete;
    PressTracker& operator=(const PressTracker&) = delete;
    ~PressTracker();

    PressResult process(const BoxF& bounds, const IInputPump::Event& event);

    void reset();

    IRenderer::Color background(const IRenderer::Color& idle, const Palette& palette) const;

private:
    inline static PressTracker* active_ = nullptr;

    bool hovered_ = false;
    bool pressed_ = false;
};

inline PressTracker::~PressTracker()
{
    if (active_ == this)
    {
        active_ = nullptr;
    }
}

inline void PressTracker::reset()
{
    if (active_ == this)
    {
        active_ = nullptr;
    }
    hovered_ = false;
    pressed_ = false;
}

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
            if (!inside)
            {
                if (active_ == this)
                {
                    active_  = nullptr;
                    pressed_ = false;
                }
                return PressResult::IGNORED;
            }

            if (active_ != nullptr && active_ != this)
            {
                pressed_ = false;
                return PressResult::IGNORED;
            }

            active_  = this;
            pressed_ = true;
            return PressResult::CONSUMED;

        case IInputPump::EventMouse::Type::LEFT_BUTTON_RELEASING:
            if (active_ != this)
            {
                pressed_ = false;
                return PressResult::IGNORED;
            }

            active_ = nullptr;
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
    if (pressed_)
    {
        return palette.pressed;
    }
    if (hovered_)
    {
        return palette.hover;
    }
    return idle;
}

void runAction(const IUIFramework::Action& action)
{
    /* Copy the callbacks first: a callback may destroy the widget that owns
     * the action (for example, a window close callback). */
    const std::vector<std::function<void(void)>> callbacks = action.callbacks;
    for (const auto& callback : callbacks)
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

struct DragState
{
    bool  dragging = false;
    Vec2F offset;
};

bool isOnEdge(const BoxF& box, const Vec2F& point, float thickness)
{
    if (!box.contains(point))
    {
        return false;
    }

    return (point.x() - box.left() <= thickness) ||
           (box.right() - point.x() <= thickness) ||
           (point.y() - box.top() <= thickness) ||
           (box.bottom() - point.y() <= thickness);
}

bool startDrag(TWidget& widget, DragState& state, const IInputPump::Event& event, const BoxF& area,
               float edge_thickness = 0.0F)
{
    const IInputPump::EventMouse* mouse = asMouse(event);
    if (!mouse ||
        mouse->type() != IInputPump::EventMouse::Type::LEFT_BUTTON_PRESSING ||
        !area.contains(mouse->cursorPos()))
    {
        return false;
    }

    if (edge_thickness > 0.0F && !isOnEdge(area, mouse->cursorPos(), edge_thickness))
    {
        return false;
    }

    state.dragging = true;
    state.offset   = mouse->cursorPos() - globalBounds(widget).topLeft();
    return true;
}

bool processDrag(TWidget& widget, DragState& state, const IInputPump::Event& event)
{
    const IInputPump::EventMouse* mouse = asMouse(event);
    if (!mouse || !state.dragging)
    {
        return false;
    }

    switch (mouse->type())
    {
        case IInputPump::EventMouse::Type::MOTION:
        {
            const Vec2F parent_origin =
                widget.parent() != nullptr ? widget.parent()->mapLocalToGlobal(Vec2F(0.0F, 0.0F)) : Vec2F(0.0F, 0.0F);
            const Vec2F new_top_left = mouse->cursorPos() - state.offset - parent_origin;
            widget.setBox(makeBox(new_top_left, widget.box().size()));
            return true;
        }

        case IInputPump::EventMouse::Type::LEFT_BUTTON_RELEASING:
            state.dragging = false;
            return true;

        default:
            return true;
    }
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
    IUIFramework::Role                  role_;
    IUIFramework::Action                on_press_;
    PressTracker                        press_;
    TObject*                            parent_ = nullptr;
};

inline ButtonObject::ButtonObject(const RenderContext& context, const BoxF& bounds, std::optional<std::string> text,
                                  std::optional<IRenderer::TextureID> icon, IUIFramework::Role role,
                                  IUIFramework::Action on_press) :
    context_(context), text_(std::move(text)), icon_(icon), role_(role), on_press_(std::move(on_press))
{
    setBox(fitToText(bounds, text_, icon_.has_value()));
    parent_ = parent();
}

inline bool ButtonObject::processEvent(const IInputPump::Event& event)
{
    if (parent() != parent_)
    {
        parent_ = parent();
        press_.reset();
    }

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
    IRenderer* renderer = acquireRenderer(context_);
    if (!renderer)
    {
        return;
    }

    const Palette palette = context_.theme->palette(role_);
    const BoxF    bounds  = globalBounds(*this);
    drawPanel(renderer, press_.background(palette.base, palette), palette.border, bounds);
    drawControlContent(renderer, context_, icon_, text_, palette.text, bounds);
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
    IUIFramework::Role                  role_;
    IUIFramework::Action                on_active_;
    IUIFramework::Action                on_deactive_;
    PressTracker                        press_;
    bool                                active_ = false;
    TObject*                            parent_ = nullptr;
};

class SwitcherPoolObject final : public IUIFramework::SwitcherPool
{
public:
    SwitcherPoolObject(const RenderContext& context, const Vec2F& min_point, const Vec2F& size,
                       IUIFramework::Direction direction, IUIFramework::Role role);

    void add(std::unique_ptr<IUIFramework::Switcher> switcher) override;

    bool processEvent(const IInputPump::Event& event) override;

    void render() override;

    void select(SwitcherObject& target);

private:
    void relayout();

    const RenderContext&    context_;
    IUIFramework::Direction direction_;
    IUIFramework::Role      role_;
    DragState               drag_;
};

inline SwitcherObject::SwitcherObject(const RenderContext& context, const BoxF& bounds, std::optional<std::string> text,
                                      std::optional<IRenderer::TextureID> icon, IUIFramework::Role role,
                                      IUIFramework::Action on_active, IUIFramework::Action on_deactive) :
    context_(context), text_(std::move(text)), icon_(icon), role_(role), on_active_(std::move(on_active)),
    on_deactive_(std::move(on_deactive))
{
    setBox(fitToText(bounds, text_, icon_.has_value()));
    parent_ = parent();
}

inline bool SwitcherObject::processEvent(const IInputPump::Event& event)
{
    if (parent() != parent_)
    {
        parent_ = parent();
        press_.reset();
    }

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
    IRenderer* renderer = acquireRenderer(context_);
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
    drawControlContent(renderer, context_, icon_, text_, text, bounds);
}

inline bool SwitcherObject::isActive() const
{
    return active_;
}

inline void SwitcherObject::activate()
{
    if (active_)
    {
        return;
    }
    active_ = true;
    runAction(on_active_);
}

inline void SwitcherObject::deactivate()
{
    if (!active_)
    {
        return;
    }
    active_ = false;
    runAction(on_deactive_);
}

inline SwitcherPoolObject::SwitcherPoolObject(const RenderContext& context, const Vec2F& min_point, const Vec2F& size,
                                              IUIFramework::Direction direction, IUIFramework::Role role) :
    context_(context), direction_(direction), role_(role)
{
    setBox(makeBox(min_point, size));
}

inline void SwitcherPoolObject::add(std::unique_ptr<IUIFramework::Switcher> switcher)
{
    addChild(std::move(switcher));
    relayout();
}

inline void SwitcherPoolObject::relayout()
{
    setBox(layoutPool(children(), box(), direction_));
}

inline bool SwitcherPoolObject::processEvent(const IInputPump::Event& event)
{
    if (processDrag(*this, drag_, event))
    {
        return true;
    }

    relayout();

    const BoxF bounds = globalBounds(*this);
    if (startDrag(*this, drag_, event, bounds, POOL_DRAG_EDGE))
    {
        return true;
    }

    if (processEventChildren(event))
    {
        return true;
    }

    return startDrag(*this, drag_, event, bounds);
}

inline void SwitcherPoolObject::render()
{
    relayout();

    IRenderer* renderer = acquireRenderer(context_);
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
        auto* switcher = dynamic_cast<SwitcherObject*>(child.get());
        if (switcher != nullptr && switcher->isActive())
        {
            switcher->deactivate();
        }
    }
    target.activate();
}

class ButtonPoolObject final : public IUIFramework::ButtonPool
{
public:
    ButtonPoolObject(const RenderContext& context, const Vec2F& min_point, const Vec2F& size,
                     IUIFramework::Direction direction, IUIFramework::Role role);

    void add(std::unique_ptr<IUIFramework::Button> button) override;

    bool processEvent(const IInputPump::Event& event) override;

    void render() override;

private:
    void relayout();

    const RenderContext&    context_;
    IUIFramework::Direction direction_;
    IUIFramework::Role      role_;
    DragState               drag_;
};

inline ButtonPoolObject::ButtonPoolObject(const RenderContext& context, const Vec2F& min_point, const Vec2F& size,
                                          IUIFramework::Direction direction, IUIFramework::Role role) :
    context_(context), direction_(direction), role_(role)
{
    setBox(makeBox(min_point, size));
}

inline void ButtonPoolObject::add(std::unique_ptr<IUIFramework::Button> button)
{
    addChild(std::move(button));
    relayout();
}

inline void ButtonPoolObject::relayout()
{
    setBox(layoutPool(children(), box(), direction_));
}

inline bool ButtonPoolObject::processEvent(const IInputPump::Event& event)
{
    if (processDrag(*this, drag_, event))
    {
        return true;
    }

    relayout();

    const BoxF bounds = globalBounds(*this);
    if (startDrag(*this, drag_, event, bounds, POOL_DRAG_EDGE))
    {
        return true;
    }

    if (processEventChildren(event))
    {
        return true;
    }

    return startDrag(*this, drag_, event, bounds);
}

inline void ButtonPoolObject::render()
{
    relayout();

    IRenderer* renderer = acquireRenderer(context_);
    if (!renderer)
    {
        return;
    }

    const Palette palette = context_.theme->palette(role_);
    drawPanel(renderer, palette.surface, palette.border, globalBounds(*this));
    renderChildren();
}

class RemoveObjectCommand final : public ACommand
{
public:
    explicit RemoveObjectCommand(TObject::ID object_id) : object_id_(object_id)
    {
    }

    void execute() const override
    {
        if (ApplicationCore::instance().findObject(object_id_) != nullptr)
        {
            ApplicationCore::instance().removeObject(object_id_);
        }
    }

private:
    TObject::ID object_id_;
};

void destroyAfterEvent(TObject::ID object_id)
{
    if (ApplicationCore::instance().findObject(object_id) == nullptr)
    {
        return;
    }
    ApplicationCore::instance().pushCommand(std::make_unique<RemoveObjectCommand>(object_id));
}

void destroyAfterEvent(const TObject& object)
{
    destroyAfterEvent(object.id());
}

class ContextMenuObject final : public IUIFramework::ContextMenu
{
public:
    ContextMenuObject(const RenderContext& context, const Vec2F& min_point, IUIFramework::Direction direction,
                      IUIFramework::Role role, std::optional<IUIFramework::EventHandler> on_event);

    void add(std::unique_ptr<IUIFramework::Button> button) override;
    void add(std::unique_ptr<IUIFramework::Switcher> switcher) override;

    bool processEvent(const IInputPump::Event& event) override;

    void render() override;

private:
    void relayout();

    bool isPressOutside(const IInputPump::Event& event) const;

    const RenderContext&                      context_;
    IUIFramework::Direction                   direction_;
    IUIFramework::Role                        role_;
    std::optional<IUIFramework::EventHandler> on_event_;
    bool                                      visible_ = true;
};

inline ContextMenuObject::ContextMenuObject(const RenderContext& context, const Vec2F& min_point,
                                            IUIFramework::Direction direction, IUIFramework::Role role,
                                            std::optional<IUIFramework::EventHandler> on_event) :
    context_(context), direction_(direction), role_(role), on_event_(std::move(on_event))
{
    setBox(makeBox(min_point, Vec2F(0.0F, 0.0F)));
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
    const bool vertical = direction_ == IUIFramework::Direction::VERTICAL;

    /* Use the largest child as the uniform item size, but keep the default menu
     * metrics as the lower bound so small items do not shrink the menu. */
    float item_width  = MENU_ITEM_WIDTH;
    float item_height = MENU_ITEM_HEIGHT;
    for (const auto& child : children())
    {
        const Vec2F child_size = static_cast<const TWidget&>(*child).box().size();
        item_width             = std::max(item_width, child_size.x());
        item_height            = std::max(item_height, child_size.y());
    }

    const auto  count     = static_cast<float>(children().size());
    const Vec2F menu_size = vertical ? Vec2F(item_width, item_height * count) : Vec2F(item_width * count, item_height);
    setBox(makeBox(box().topLeft(), menu_size));

    float offset = 0.0F;
    for (auto& child : children())
    {
        const Vec2F position = vertical ? Vec2F(0.0F, offset) : Vec2F(offset, 0.0F);
        static_cast<TWidget&>(*child).setBox(makeBox(position, Vec2F(item_width, item_height)));
        offset += vertical ? item_height : item_width;
    }
}

inline bool ContextMenuObject::processEvent(const IInputPump::Event& event)
{
    if (!visible_)
    {
        return false;
    }

    relayout();

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
        destroyAfterEvent(*this);
    }
    return processEventChildren(event);
}

inline void ContextMenuObject::render()
{
    if (!visible_)
    {
        return;
    }

    relayout();

    IRenderer* renderer = acquireRenderer(context_);
    if (!renderer)
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
    WindowMenuObject(const RenderContext& context, const BoxF& bounds, std::optional<std::string> label,
                     IUIFramework::Role role, std::optional<IUIFramework::EventHandler> on_event,
                     std::optional<IUIFramework::Action> on_close);

    void add(std::unique_ptr<IUIFramework::Button> button) override;
    void add(std::unique_ptr<IUIFramework::Switcher> switcher) override;
    void add(std::unique_ptr<IUIFramework::ContextMenu> menu) override;
    void add(std::unique_ptr<IUIFramework::ButtonPool> pool) override;
    void add(std::unique_ptr<IUIFramework::SwitcherPool> pool) override;

    bool processEvent(const IInputPump::Event& event) override;

    void render() override;

private:
    void relayout();

    const RenderContext&                      context_;
    std::optional<std::string>                label_;
    IUIFramework::Role                        role_;
    std::optional<IUIFramework::EventHandler> on_event_;
    ButtonObject*                             close_button_ = nullptr;
    Vec2F                                     close_size_   = Vec2F(CLOSE_BUTTON_SIZE, CLOSE_BUTTON_SIZE);
    DragState                                 drag_;
};

inline WindowMenuObject::WindowMenuObject(const RenderContext& context, const BoxF& bounds,
                                          std::optional<std::string> label, IUIFramework::Role role,
                                          std::optional<IUIFramework::EventHandler> on_event,
                                          std::optional<IUIFramework::Action>       on_close) :
    context_(context), label_(std::move(label)), role_(role), on_event_(std::move(on_event))
{
    setBox(bounds);

    if (on_close)
    {
        IUIFramework::Action close_action = std::move(*on_close);
        const TObject::ID    window_id    = id();
        close_action.callbacks.emplace_back([window_id]() { destroyAfterEvent(window_id); });

        auto close    = std::make_unique<ButtonObject>(context_, BoxF(Vec2F(CLOSE_BUTTON_SIZE, CLOSE_BUTTON_SIZE)),
                                                       std::optional<std::string>("X"), std::nullopt, role_,
                                                       std::move(close_action));
        close_size_   = close->box().size();
        close_button_ = close.get();
        addChild(std::move(close));
    }

    const float close_width = close_button_ != nullptr ? close_size_.x() : 0.0F;
    setBox(fitWindowBox(bounds, label_, close_width));
    relayout();
}

inline void WindowMenuObject::relayout()
{
    if (close_button_ == nullptr)
    {
        return;
    }

    const Vec2F size      = box().size();
    const Vec2F close_pos = Vec2F(size.x() - close_size_.x() - PADDING, (HEADER_HEIGHT - close_size_.y()) / 2.0F);
    close_button_->setBox(makeBox(close_pos, close_size_));
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
    if (processDrag(*this, drag_, event))
    {
        return true;
    }

    relayout();

    if (on_event_ && (*on_event_)(event))
    {
        return true;
    }

    if (processEventChildren(event))
    {
        return true;
    }

    const BoxF window = globalBounds(*this);
    const BoxF header = makeBox(window.topLeft(), Vec2F(window.size().x(), HEADER_HEIGHT));
    return startDrag(*this, drag_, event, header);
}

inline void WindowMenuObject::render()
{
    relayout();

    IRenderer* renderer = acquireRenderer(context_);
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
        const float reserved  = close_button_ != nullptr ? close_size_.x() + 2.0F * PADDING : 0.0F;
        const BoxF  label_box = makeBox(header_box.topLeft(), Vec2F(header_box.size().x() - reserved, HEADER_HEIGHT));
        drawText(renderer, context_, *label_, palette.text, label_box);
    }

    renderChildren();
}

} // namespace

inline UIWi& UIWi::instance()
{
    static UIWi framework;
    return framework;
}

inline UIWi::UIWi()
{
    registryTheme(DEFAULT_THEME_NAME, IRenderer::Color{53, 132, 228, OPAQUE_ALPHA},
                  IRenderer::Color{222, 221, 218, OPAQUE_ALPHA}, IRenderer::Color{246, 245, 244, OPAQUE_ALPHA},
                  IRenderer::Color{28, 113, 216, OPAQUE_ALPHA}, IRenderer::Color{224, 27, 36, OPAQUE_ALPHA});
    registryTheme(DARK_THEME_NAME, IRenderer::Color{53, 132, 228, OPAQUE_ALPHA},
                  IRenderer::Color{61, 61, 61, OPAQUE_ALPHA}, IRenderer::Color{36, 36, 36, OPAQUE_ALPHA},
                  IRenderer::Color{98, 160, 234, OPAQUE_ALPHA}, IRenderer::Color{255, 123, 107, OPAQUE_ALPHA});
    setTheme(DEFAULT_THEME_NAME);
    loadFonts();
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
    return std::make_unique<ContextMenuObject>(context_, description.min_point, description.direction, description.role,
                                               std::move(description.on_event));
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
    return std::make_unique<WindowMenuObject>(context_, description.box, std::move(description.label), description.role,
                                              std::move(description.on_event), std::move(description.on_close));
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

inline void UIWi::loadFonts()
{
    IRenderer* renderer = queryRenderer();
    if (!renderer)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::WARNING,
                             "UI font is not loaded: IRenderer interface is not registered");
        return;
    }

    const std::optional<std::filesystem::path> list_path = fontListPath();
    if (!list_path)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "UI font list location is not resolved");
        return;
    }

    for (const std::string& font : readFontList(*list_path))
    {
        const std::optional<IRenderer::FontID> loaded = renderer->loadFont(font);
        if (loaded)
        {
            context_.font = loaded;
            ApplicationCore::log(ApplicationCore::LogLevel::OK, "UI font: %s", font.c_str());
            return;
        }
    }

    ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "No UI font from '%s' is loaded",
                         list_path->string().c_str());
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

} // namespace Kabuto

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    IRenderer* renderer = nullptr;
    if (ApplicationCore::instance().queryInterface(IID_RENDERER, reinterpret_cast<void**>(&renderer)) !=
            ApplicationCore::Error::SUCCESS ||
        renderer == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "UI framework can't find the renderer");
        return false;
    }

    Kabuto::UI::UIWi& framework = Kabuto::UI::UIWi::instance();

    const ApplicationCore::Error error = ApplicationCore::instance().registerInterface(
        Kabuto::IID_IUIFramework, reinterpret_cast<void*>(static_cast<Kabuto::IUIFramework*>(&framework))
    );
    if (error != ApplicationCore::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Failed to register IUIFramework: %s",
                             ApplicationCore::strError(error));
        return false;
    }

    return true;
}
