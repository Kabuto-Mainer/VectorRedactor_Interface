#include "SDK/Core/ApplicationCore.hpp"
#include "SDK/Interfaces/IRenderer.hpp"
#include "SDK/Interfaces/IUIFramework.hpp"
#include "SDK/Utils/ACommand.hpp"
#include "SDK/Utils/AObject.hpp"
#include "SDK/Utils/BoxF.hpp"
#include "SDK/Utils/Event.hpp"
#include "SDK/Utils/JsonParser.hpp"
#include "SDK/Utils/Vec2F.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <memory>
#include <optional>
#include <sstream>
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
constexpr float   LINE_SPACING         = 1.0F;
constexpr int     FONT_SIZE            = 16;
constexpr char    DEFAULT_THEME_NAME[] = "default";
constexpr char    DARK_THEME_NAME[]    = "dark";
constexpr char    ROLE_KEY_PREFIX[]    = "roles.";

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
    Color_t base;
    Color_t hover;
    Color_t pressed;
    Color_t text;
    Color_t border;
    Color_t surface;
    Color_t surface_text;
    Color_t canvas;
};

Color_t mix(const Color_t& from, const Color_t& to, float ratio)
{
    const auto channel = [ratio](uint8_t from_value, uint8_t to_value)
    {
        return static_cast<uint8_t>(static_cast<float>(from_value) +
                                    (static_cast<float>(to_value) - static_cast<float>(from_value)) * ratio);
    };
    return Color_t(channel(from.r, to.r), channel(from.g, to.g), channel(from.b, to.b), OPAQUE_ALPHA);
}

float luminance(const Color_t& color)
{
    return (0.2126F * color.r + 0.7152F * color.g + 0.0722F * color.b) / CHANNEL_MAX;
}

Color_t contrastOf(const Color_t& color)
{
    if (luminance(color) < LIGHTNESS_THRESHOLD)
    {
        return Color_t(255, 255, 255, OPAQUE_ALPHA);
    }
    return Color_t(0, 0, 0, OPAQUE_ALPHA);
}

Palette derive(const Color_t& base, const Color_t& surface, const Color_t& canvas)
{
    const Color_t text    = contrastOf(base);
    const Color_t panel   = mix(surface, base, SECONDARY_TINT);
    const Color_t tinted  = mix(canvas, base, CANVAS_TINT);
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
    Color_t              window;
    Color_t              surface;
    Color_t              accent;
    std::vector<Color_t> role_bases;

    Palette palette(Role role) const
    {
        const Color_t& base = role < role_bases.size() ? role_bases[role] : role_bases[Roles::Default];
        return derive(base, surface, window);
    }
};

Theme makeTheme(const Color_t& primary, const Color_t& secondary, const Color_t& neutral, const Color_t& accent,
                const Color_t& unical)
{
    const bool    dark    = luminance(neutral) < LIGHTNESS_THRESHOLD;
    const Color_t white(255, 255, 255, OPAQUE_ALPHA);
    const Color_t surface = mix(neutral, white, dark ? DARK_SURFACE_LIFT : LIGHT_SURFACE_LIFT);
    const Color_t button  = mix(neutral, secondary, SECONDARY_TINT);

    return Theme{neutral, surface, accent, {button, primary, unical}};
}

struct RenderContext
{
    const Theme*                     theme = nullptr;
    std::optional<IRenderer::FontID> font;
};

IRenderer* queryRenderer()
{
    void* raw = nullptr;
    if (FAILED(ApplicationCore::instance().getInterface(IID_IRender, &raw)))
    {
        return nullptr;
    }
    return static_cast<IRenderer*>(raw);
}

const MouseEvent* asMouse(const Event& event)
{
    return std::get_if<MouseEvent>(&event.data);
}

Role resolveRole(const std::optional<Role>& role)
{
    return role.value_or(Roles::Default);
}

std::optional<IRenderer::ResourceID> loadIcon(const std::optional<std::string>& path)
{
    IRenderer* renderer = queryRenderer();
    if (!path || !renderer)
    {
        return std::nullopt;
    }
    return renderer->loadTexture(path->c_str());
}

void drawPanel(IRenderer* renderer, const Color_t& fill, const Color_t& border, const BoxF& box)
{
    renderer->renderRect(fill, box, NO_ANGLE);
    renderer->renderRectLines(border, box, NO_ANGLE);
}

void drawText(IRenderer* renderer, const RenderContext& context, const std::string& text, const Color_t& color,
              const BoxF& box, TextHAlign h_align)
{
    if (!context.font)
    {
        return;
    }
    const TextFormat format{color,   *context.font,      FONT_SIZE, TextOverflowAction::TailReplace,
                            h_align, TextWAlign::Center, PADDING,   LINE_SPACING,
                            false};
    renderer->renderText(text.c_str(), format, box);
}

void drawContent(IRenderer* renderer, const RenderContext& context, const std::optional<IRenderer::ResourceID>& icon,
                 const std::optional<std::string>& text, const Color_t& color, const BoxF& box)
{
    if (icon)
    {
        renderer->renderTexture(*icon, BoxF(box.pos + Vec2F(PADDING, PADDING), box.size - Vec2F(2.0F * PADDING)),
                                NO_ANGLE);
    }
    if (text)
    {
        drawText(renderer, context, *text, color, box, TextHAlign::Center);
    }
}

Vec2F menuSize(Direct direct, std::size_t count)
{
    const auto items = static_cast<float>(count);
    return direct == Direct::Vertical ? Vec2F(MENU_ITEM_WIDTH, MENU_ITEM_HEIGHT * items) :
                                        Vec2F(MENU_ITEM_WIDTH * items, MENU_ITEM_HEIGHT);
}

using Placement = std::optional<BoxF>;

std::vector<Placement> slotsFor(const BoxF& bounds, Direct direct, std::size_t count)
{
    std::vector<Placement> slots;
    if (count == 0)
    {
        return slots;
    }

    const auto  divider = static_cast<float>(count);
    const Vec2F slot    = direct == Direct::Vertical ? Vec2F(bounds.size.x, bounds.size.y / divider) :
                                                       Vec2F(bounds.size.x / divider, bounds.size.y);
    const Vec2F step    = direct == Direct::Vertical ? Vec2F(0.0F, slot.y) : Vec2F(slot.x, 0.0F);

    Vec2F position = bounds.pos;
    for (std::size_t i = 0; i < count; ++i)
    {
        slots.emplace_back(BoxF(position, slot));
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
    PressResult process(const BoxF& bounds, const Event& event)
    {
        const MouseEvent* mouse = asMouse(event);
        if (!mouse)
        {
            return PressResult::IGNORED;
        }

        const bool inside = bounds.contains(mouse->pos);
        hovered_          = inside;

        switch (event.type)
        {
            case EventType::MOUSE_DOWN:
                if (inside && mouse->button == MouseButton::LEFT)
                {
                    pressed_ = true;
                    return PressResult::CONSUMED;
                }
                return PressResult::IGNORED;

            case EventType::MOUSE_UP:
                if (!pressed_ || mouse->button != MouseButton::LEFT)
                {
                    return PressResult::IGNORED;
                }
                pressed_ = false;
                return inside ? PressResult::CLICKED : PressResult::CONSUMED;

            default:
                return PressResult::IGNORED;
        }
    }

    Color_t background(const Color_t& idle, const Palette& palette) const
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

private:
    bool hovered_ = false;
    bool pressed_ = false;
};

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

class ButtonObject final : public AObject
{
public:
    ButtonObject(const RenderContext& context, const BoxF& bounds, std::optional<std::string> text,
                 std::optional<IRenderer::ResourceID> icon, Role role, Action on_press) :
        context_(context), bounds_(bounds), text_(std::move(text)), icon_(icon), role_(role),
        on_press_(std::move(on_press))
    {
    }

    bool processEvent(const Event& event) override
    {
        switch (press_.process(bounds_, event))
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

    void render() override
    {
        IRenderer* renderer = queryRenderer();
        if (!renderer)
        {
            return;
        }

        const Palette palette = context_.theme->palette(role_);
        drawPanel(renderer, press_.background(palette.base, palette), palette.border, bounds_);
        drawContent(renderer, context_, icon_, text_, palette.text, bounds_);
    }

private:
    const RenderContext&                 context_;
    BoxF                                 bounds_;
    std::optional<std::string>           text_;
    std::optional<IRenderer::ResourceID> icon_;
    Role                                 role_;
    Action                               on_press_;
    PressTracker                         press_;
};

class SwitcherObject final : public AObject
{
public:
    SwitcherObject(const RenderContext& context, const BoxF& bounds, std::optional<std::string> text,
                   std::optional<IRenderer::ResourceID> icon, Role role, Action on_active, Action on_deactive) :
        context_(context), bounds_(bounds), text_(std::move(text)), icon_(icon), role_(role),
        on_active_(std::move(on_active)), on_deactive_(std::move(on_deactive))
    {
    }

    bool processEvent(const Event& event) override
    {
        switch (press_.process(bounds_, event))
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

    void render() override
    {
        IRenderer* renderer = queryRenderer();
        if (!renderer)
        {
            return;
        }

        const Palette palette = context_.theme->palette(role_);
        const Color_t idle    = active_ ? palette.base : palette.surface;
        const Color_t text    = active_ ? palette.text : palette.surface_text;
        const Color_t border  = active_ ? context_.theme->accent : palette.border;
        drawPanel(renderer, press_.background(idle, palette), border, bounds_);
        drawContent(renderer, context_, icon_, text_, text, bounds_);
    }

private:
    const RenderContext&                 context_;
    BoxF                                 bounds_;
    std::optional<std::string>           text_;
    std::optional<IRenderer::ResourceID> icon_;
    Role                                 role_;
    Action                               on_active_;
    Action                               on_deactive_;
    PressTracker                         press_;
    bool                                 active_ = false;
};

class ContextMenuObject final : public AObject
{
public:
    ContextMenuObject(const RenderContext& context, const BoxF& bounds, Role role,
                      std::optional<EventHandler> on_event) :
        context_(context), bounds_(bounds), role_(role), on_event_(std::move(on_event))
    {
    }

    bool processEvent(const Event& event) override
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
        return AObject::processEvent(event);
    }

    void render() override
    {
        IRenderer* renderer = queryRenderer();
        if (!visible_ || !renderer)
        {
            return;
        }

        const Palette palette = context_.theme->palette(role_);
        drawPanel(renderer, palette.surface, palette.border, bounds_);
        AObject::render();
    }

private:
    bool isPressOutside(const Event& event) const
    {
        const MouseEvent* mouse = asMouse(event);
        return event.type == EventType::MOUSE_DOWN && mouse && !bounds_.contains(mouse->pos);
    }

    const RenderContext&        context_;
    BoxF                        bounds_;
    Role                        role_;
    std::optional<EventHandler> on_event_;
    bool                        visible_ = true;
};

class PoolMenuObject final : public AObject
{
public:
    PoolMenuObject(const RenderContext& context, const BoxF& bounds, Role role) :
        context_(context), bounds_(bounds), role_(role)
    {
    }

    void render() override
    {
        IRenderer* renderer = queryRenderer();
        if (!renderer)
        {
            return;
        }

        const Palette palette = context_.theme->palette(role_);
        drawPanel(renderer, palette.surface, palette.border, bounds_);
        AObject::render();
    }

private:
    const RenderContext& context_;
    BoxF                 bounds_;
    Role                 role_;
};

class WindowObject final : public AObject
{
public:
    WindowObject(const RenderContext& context, const BoxF& bounds, std::optional<std::string> label, Role role,
                 std::optional<EventHandler> on_event) :
        context_(context), bounds_(bounds), label_(std::move(label)), role_(role), on_event_(std::move(on_event))
    {
    }

    bool processEvent(const Event& event) override
    {
        if (on_event_ && (*on_event_)(event))
        {
            return true;
        }
        return AObject::processEvent(event);
    }

    void render() override
    {
        IRenderer* renderer = queryRenderer();
        if (!renderer)
        {
            return;
        }

        const Palette palette    = context_.theme->palette(role_);
        const BoxF    header_box(bounds_.pos, Vec2F(bounds_.size.x, HEADER_HEIGHT));

        drawPanel(renderer, palette.canvas, palette.border, bounds_);
        drawPanel(renderer, palette.base, palette.border, header_box);
        renderer->renderRect(context_.theme->accent,
                             BoxF(Vec2F(bounds_.pos.x, bounds_.pos.y + HEADER_HEIGHT - ACCENT_LINE_HEIGHT),
                                  Vec2F(bounds_.size.x, ACCENT_LINE_HEIGHT)),
                             NO_ANGLE);

        if (label_)
        {
            const BoxF label_box(header_box.pos, Vec2F(header_box.size.x - CLOSE_AREA_WIDTH, HEADER_HEIGHT));
            drawText(renderer, context_, *label_, palette.text, label_box, TextHAlign::Left);
        }

        AObject::render();
    }

private:
    const RenderContext&        context_;
    BoxF                        bounds_;
    std::optional<std::string>  label_;
    Role                        role_;
    std::optional<EventHandler> on_event_;
};

class Builder final : public ADescriptionVisitor
{
public:
    Builder(const RenderContext& context, Placement placement) : context_(context), placement_(placement)
    {
    }

    std::unique_ptr<AObject> take()
    {
        return std::move(result_);
    }

    bool doForButton(ADescription& description) override
    {
        auto& button = static_cast<ButtonDescription&>(description);
        result_ =
            std::make_unique<ButtonObject>(context_, placement_.value_or(button.box), std::move(button.text),
                                           loadIcon(button.icon), resolveRole(button.role), std::move(button.on_press));
        return true;
    }

    bool doForSwitcher(ADescription& description) override
    {
        auto& switcher = static_cast<SwitcherDescription&>(description);
        result_ =
            std::make_unique<SwitcherObject>(context_, placement_.value_or(switcher.box), std::move(switcher.text),
                                             loadIcon(switcher.icon), resolveRole(switcher.role),
                                             std::move(switcher.on_active), std::move(switcher.on_deactive));
        return true;
    }

    bool doForContextMenu(ADescription& description) override
    {
        auto&             menu   = static_cast<ContextMenu&>(description);
        const std::size_t count  = menu.objects.size();
        const BoxF        bounds = placement_.value_or(BoxF(menu.min_point, menuSize(menu.direct, count)));

        auto object =
            std::make_unique<ContextMenuObject>(context_, bounds, resolveRole(menu.role), std::move(menu.on_event));
        if (!addChildren(*object, menu.objects, slotsFor(bounds, menu.direct, count)))
        {
            return false;
        }
        result_ = std::move(object);
        return true;
    }

    bool doForPoolMenu(ADescription& description) override
    {
        auto&      pool   = static_cast<PoolMenu&>(description);
        const BoxF bounds = placement_.value_or(BoxF(pool.min_point, pool.size));

        auto object = std::make_unique<PoolMenuObject>(context_, bounds, resolveRole(pool.role));
        if (!addChildren(*object, pool.objects, slotsFor(bounds, pool.direct, pool.objects.size())))
        {
            return false;
        }
        result_ = std::move(object);
        return true;
    }

    bool doForWindow(ADescription& description) override
    {
        auto&      window = static_cast<Window&>(description);
        const BoxF bounds = placement_.value_or(window.box);
        const Role role   = resolveRole(window.role);

        auto object = std::make_unique<WindowObject>(context_, bounds, std::move(window.label), role,
                                                     std::move(window.on_event));

        if (window.on_close)
        {
            const Vec2F close_pos(bounds.pos.x + bounds.size.x - CLOSE_BUTTON_SIZE - PADDING, bounds.pos.y + PADDING);
            object->addChild(std::make_unique<ButtonObject>(
                context_, BoxF(close_pos, Vec2F(CLOSE_BUTTON_SIZE, CLOSE_BUTTON_SIZE)), std::optional<std::string>("X"),
                std::nullopt, role, std::move(*window.on_close)
            ));
        }

        if (!addChildren(*object, window.objects, std::vector<Placement>(window.objects.size())))
        {
            return false;
        }
        result_ = std::move(object);
        return true;
    }

private:
    bool addChildren(AObject& parent, std::vector<std::unique_ptr<ADescription>>& objects,
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

    const RenderContext&     context_;
    Placement                placement_;
    std::unique_ptr<AObject> result_;
};

int hexDigit(char c)
{
    if (c >= '0' && c <= '9')
    {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f')
    {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F')
    {
        return c - 'A' + 10;
    }
    return -1;
}

std::optional<Color_t> parseColor(const std::string& value)
{
    const std::size_t start = (!value.empty() && value[0] == '#') ? 1 : 0;
    if (value.size() - start != 6)
    {
        return std::nullopt;
    }

    uint8_t channels[3] = {0, 0, 0};
    for (std::size_t i = 0; i < 3; ++i)
    {
        const int high = hexDigit(value[start + 2 * i]);
        const int low  = hexDigit(value[start + 2 * i + 1]);
        if (high < 0 || low < 0)
        {
            return std::nullopt;
        }
        channels[i] = static_cast<uint8_t>(high * 16 + low);
    }
    return Color_t(channels[0], channels[1], channels[2], OPAQUE_ALPHA);
}

Theme buildTheme(const std::vector<Color_t>& role_defaults, const Color_t& primary, const Color_t& secondary,
                 const Color_t& neutral, const Color_t& accent, const Color_t& unical)
{
    Theme theme = makeTheme(primary, secondary, neutral, accent, unical);
    for (std::size_t i = theme.role_bases.size(); i < role_defaults.size(); ++i)
    {
        theme.role_bases.push_back(role_defaults[i]);
    }
    return theme;
}

Role ensureRole(std::vector<std::string>& role_names, std::vector<Color_t>& role_defaults,
                std::unordered_map<std::string, Theme>& themes, const std::string& name, const Color_t& base)
{
    const auto it = std::find(role_names.begin(), role_names.end(), name);
    if (it != role_names.end())
    {
        return static_cast<Role>(std::distance(role_names.begin(), it));
    }

    role_names.push_back(name);
    role_defaults.push_back(base);
    for (auto& entry : themes)
    {
        entry.second.role_bases.push_back(base);
    }
    return static_cast<Role>(role_names.size() - 1);
}

} // namespace

class UIWi final : public IFramework
{
public:
    UIWi()
    {
        role_names_ = {"default", "primary", "danger"};
        role_defaults_.resize(role_names_.size(), Color_t(0, 0, 0, OPAQUE_ALPHA));

        registryTheme(DEFAULT_THEME_NAME, Color_t(53, 132, 228, OPAQUE_ALPHA), Color_t(222, 221, 218, OPAQUE_ALPHA),
                      Color_t(246, 245, 244, OPAQUE_ALPHA), Color_t(28, 113, 216, OPAQUE_ALPHA),
                      Color_t(224, 27, 36, OPAQUE_ALPHA));
        registryTheme(DARK_THEME_NAME, Color_t(53, 132, 228, OPAQUE_ALPHA), Color_t(61, 61, 61, OPAQUE_ALPHA),
                      Color_t(36, 36, 36, OPAQUE_ALPHA), Color_t(98, 160, 234, OPAQUE_ALPHA),
                      Color_t(255, 123, 107, OPAQUE_ALPHA));
        setTheme(DEFAULT_THEME_NAME);
    }

    UIWi(const UIWi&)            = delete;
    UIWi& operator=(const UIWi&) = delete;

    HRESULT queryInterface(const IID& iid, void** ppv) override
    {
        if (!ppv)
        {
            return E_POINTER;
        }
        *ppv = nullptr;

        if (iid != IID_IUnknown && iid != IID_IPlugin && iid != IID_IUIFramework)
        {
            return E_NOINTERFACE;
        }
        *ppv = static_cast<IFramework*>(this);
        addRef();
        return S_OK;
    }

    ULONG addRef() override
    {
        return ++ref_count_;
    }

    ULONG release() override
    {
        const ULONG count = --ref_count_;
        if (count == 0)
        {
            delete this;
        }
        return count;
    }

    HRESULT init(IApplicationCore* /* core */) override
    {
        return S_OK;
    }

    HRESULT shutdown() override
    {
        return S_OK;
    }

    std::unique_ptr<AObject> create(ADescription& description) override
    {
        Builder builder(context_, std::nullopt);
        if (!description.accept(builder))
        {
            return nullptr;
        }
        return builder.take();
    }

    void registryTheme(std::string name, const Color_t& primary, const Color_t& secondary, const Color_t& neutral,
                       const Color_t& accent, const Color_t& unical) override
    {
        themes_.insert_or_assign(std::move(name),
                                 buildTheme(role_defaults_, primary, secondary, neutral, accent, unical));
    }

    bool setTheme(std::string name) override
    {
        const auto it = themes_.find(name);
        if (it == themes_.end())
        {
            return false;
        }
        context_.theme = &it->second;
        return true;
    }

    bool setFont(std::string path) override
    {
        IRenderer* renderer = queryRenderer();
        if (!renderer)
        {
            return false;
        }
        context_.font = renderer->loadFont(path.c_str());
        return true;
    }

    Role registerRole(std::string name, const Color_t& base) override
    {
        const Role id = ensureRole(role_names_, role_defaults_, themes_, name, base);
        role_defaults_[id] = base;
        for (auto& entry : themes_)
        {
            entry.second.role_bases[id] = base;
        }
        return id;
    }

    bool importTheme(std::string path) override
    {
        std::ifstream file(path);
        if (!file)
        {
            return false;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        const std::string text = buffer.str();

        Json::Fields fields;
        if (!Json::parse(text, fields))
        {
            return false;
        }

        const auto readColor = [&fields](const char* key) -> std::optional<Color_t>
        {
            const auto it = fields.find(key);
            return it == fields.end() ? std::nullopt : parseColor(it->second);
        };

        const auto name_it   = fields.find("name");
        const auto primary   = readColor("primary");
        const auto secondary = readColor("secondary");
        const auto neutral   = readColor("neutral");
        const auto accent    = readColor("accent");
        const auto unical    = readColor("unical");
        if (name_it == fields.end() || !primary || !secondary || !neutral || !accent || !unical)
        {
            return false;
        }

        const std::string                     prefix(ROLE_KEY_PREFIX);
        std::vector<std::pair<Role, Color_t>> overrides;
        for (const auto& [key, value] : fields)
        {
            if (key.compare(0, prefix.size(), prefix) != 0)
            {
                continue;
            }
            const auto base = parseColor(value);
            if (!base)
            {
                return false;
            }
            overrides.emplace_back(ensureRole(role_names_, role_defaults_, themes_, key.substr(prefix.size()), *base),
                                   *base);
        }

        Theme theme = buildTheme(role_defaults_, *primary, *secondary, *neutral, *accent, *unical);
        for (const auto& [role, base] : overrides)
        {
            theme.role_bases[role] = base;
        }
        themes_.insert_or_assign(name_it->second, std::move(theme));
        return true;
    }

private:
    RenderContext                          context_;
    std::unordered_map<std::string, Theme> themes_;
    std::vector<std::string>               role_names_;
    std::vector<Color_t>                   role_defaults_;
    ULONG                                  ref_count_ = 1;
};

} // namespace UI

extern "C" IUnknown* getInterface()
{
    return new UI::UIWi();
}
