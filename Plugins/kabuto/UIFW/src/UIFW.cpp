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
#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <json/json.h>
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

constexpr uint8_t OPAQUE_ALPHA      = 255;
constexpr float   CHANNEL_MAX       = 255.0F;
constexpr float   NO_ANGLE          = 0.0F;
constexpr char    CONFIG_FILENAME[] = "uifw.json";

constexpr char DEFAULT_THEME_NAME[] = "default";

struct PaletteStyle
{
    float lightness_threshold = 0.5F;
    float secondary_tint      = 0.15F;
    float canvas_tint         = 0.04F;
    float hover_mix           = 0.08F;
    float pressed_mix         = 0.16F;
    float border_mix          = 0.20F;
    float dark_surface_lift   = 0.08F;
    float light_surface_lift  = 0.50F;
};

struct Style
{
    int          font_size          = 16;
    float        text_width_factor  = 0.6F;
    float        padding            = 4.0F;
    float        header_height      = 28.0F;
    float        accent_line_height = 2.0F;
    float        close_button_size  = 20.0F;
    float        menu_item_width    = 120.0F;
    float        menu_item_height   = 28.0F;
    float        pool_gap           = 4.0F;
    float        pool_drag_edge     = 2.0F;
    PaletteStyle palette;
};

struct ThemeDescription
{
    std::string      name;
    IRenderer::Color primary;
    IRenderer::Color secondary;
    IRenderer::Color neutral;
    IRenderer::Color accent;
    IRenderer::Color unical;
};

struct Settings
{
    Style                         style;
    std::string                   default_theme = DEFAULT_THEME_NAME;
    std::vector<ThemeDescription> themes;
    std::vector<std::string>      fonts;
};

IRenderer::Color makeColor(uint8_t r, uint8_t g, uint8_t b)
{
    return IRenderer::Color{r, g, b, OPAQUE_ALPHA};
}

std::vector<ThemeDescription> defaultThemes()
{
    return {
        {DEFAULT_THEME_NAME,
         makeColor(53, 132, 228),
         makeColor(222, 221, 218),
         makeColor(246, 245, 244),
         makeColor(28, 113, 216),
         makeColor(224, 27, 36)},
        {"dark",
         makeColor(53, 132, 228),
         makeColor(61, 61, 61),
         makeColor(36, 36, 36),
         makeColor(98, 160, 234),
         makeColor(255, 123, 107)},
    };
}

std::vector<std::string> defaultFonts()
{
    return {
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/Adwaita/AdwaitaSans-Regular.ttf",
    };
}

Settings defaultSettings()
{
    Settings settings;
    settings.themes = defaultThemes();
    settings.fonts  = defaultFonts();
    return settings;
}

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

IRenderer::Color contrastOf(const IRenderer::Color& color, float lightness_threshold)
{
    if (luminance(color) < lightness_threshold)
    {
        return IRenderer::Color{255, 255, 255, OPAQUE_ALPHA};
    }
    return IRenderer::Color{0, 0, 0, OPAQUE_ALPHA};
}

Palette derive(const IRenderer::Color& base, const IRenderer::Color& surface, const IRenderer::Color& canvas,
               const PaletteStyle& style)
{
    const IRenderer::Color text   = contrastOf(base, style.lightness_threshold);
    const IRenderer::Color panel  = mix(surface, base, style.secondary_tint);
    const IRenderer::Color tinted = mix(canvas, base, style.canvas_tint);
    return Palette{base,
                   mix(base, text, style.hover_mix),
                   mix(base, text, style.pressed_mix),
                   text,
                   mix(base, text, style.border_mix),
                   panel,
                   contrastOf(panel, style.lightness_threshold),
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

Theme makeTheme(const ThemeDescription& description, const PaletteStyle& style)
{
    const bool dark = luminance(description.neutral) < style.lightness_threshold;
    const IRenderer::Color white{255, 255, 255, OPAQUE_ALPHA};
    const IRenderer::Color surface = mix(description.neutral, white,
                                         dark ? style.dark_surface_lift : style.light_surface_lift);
    const IRenderer::Color button = mix(description.neutral, description.secondary, style.secondary_tint);

    const std::array<IRenderer::Color, static_cast<std::size_t>(IUIFramework::Role::MAX)> role_bases{
        button, description.primary, description.unical};

    std::array<Palette, static_cast<std::size_t>(IUIFramework::Role::MAX)> palettes{};
    for (std::size_t index = 0; index < role_bases.size(); ++index)
    {
        palettes[index] = derive(role_bases[index], surface, description.neutral, style);
    }

    return Theme{description.neutral, surface, description.accent, palettes};
}

struct RenderContext
{
    const Theme*                     theme   = nullptr;
    const Style*                     style   = nullptr;
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

TextMetrics measureText(const std::string& text, const Style& style)
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

    return TextMetrics{static_cast<float>(max_line) * static_cast<float>(style.font_size) * style.text_width_factor,
                       static_cast<float>(lines) * static_cast<float>(style.font_size)};
}

BoxF ensureMinSize(const BoxF& box, float min_width, float min_height)
{
    const Vec2F size   = box.size();
    const Vec2F origin = box.topLeft();
    return makeBox(origin, Vec2F(std::max(size.x(), min_width), std::max(size.y(), min_height)));
}

BoxF fitToText(const BoxF& box, const std::optional<std::string>& text, bool has_icon, const Style& style)
{
    if (!text || text->empty())
    {
        return box;
    }

    const TextMetrics metrics    = measureText(*text, style);
    const float       icon_width = has_icon ? static_cast<float>(style.font_size) + style.padding : 0.0F;
    const float       min_width  = metrics.width + 2.0F * style.padding + icon_width;
    const float       min_height = metrics.height + 2.0F * style.padding;
    return ensureMinSize(box, min_width, min_height);
}

BoxF fitWindowBox(const BoxF& box, const std::optional<std::string>& label, float close_width, const Style& style)
{
    const TextMetrics metrics = (label && !label->empty()) ? measureText(*label, style) : TextMetrics{0.0F, 0.0F};
    const float       reserved  = close_width > 0.0F ? close_width + 2.0F * style.padding : 0.0F;
    const float       min_width = metrics.width + 2.0F * style.padding + reserved;
    return ensureMinSize(box, min_width, style.header_height);
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

    Settings                               settings_;
    RenderContext                          context_;
    std::unordered_map<std::string, Theme> themes_;
};

namespace
{

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

std::optional<std::filesystem::path> configFilePath()
{
    const std::optional<std::filesystem::path> directory =
        moduleDirectory(reinterpret_cast<const void*>(&configFilePath));
    if (!directory)
    {
        return std::nullopt;
    }
    return *directory / CONFIG_FILENAME;
}

void logInvalidConfigField(const std::filesystem::path& path, const char* field)
{
    ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "UI config '%s': invalid '%s', using default",
                         path.string().c_str(), field);
}

int hexDigit(char value)
{
    if (value >= '0' && value <= '9')
    {
        return value - '0';
    }
    if (value >= 'a' && value <= 'f')
    {
        return value - 'a' + 10;
    }
    if (value >= 'A' && value <= 'F')
    {
        return value - 'A' + 10;
    }
    return -1;
}

std::optional<uint8_t> parseByte(const std::string& text, std::size_t index)
{
    const int high = hexDigit(text[index]);
    const int low  = hexDigit(text[index + 1]);
    if (high < 0 || low < 0)
    {
        return std::nullopt;
    }
    return static_cast<uint8_t>(high * 16 + low);
}

std::optional<IRenderer::Color> parseColor(const Json::Value& value)
{
    if (!value.isString())
    {
        return std::nullopt;
    }

    const std::string text = value.asString();
    if ((text.size() != 7 && text.size() != 9) || text.front() != '#')
    {
        return std::nullopt;
    }

    const std::optional<uint8_t> red   = parseByte(text, 1);
    const std::optional<uint8_t> green = parseByte(text, 3);
    const std::optional<uint8_t> blue  = parseByte(text, 5);
    if (!red || !green || !blue)
    {
        return std::nullopt;
    }

    uint8_t alpha = OPAQUE_ALPHA;
    if (text.size() == 9)
    {
        const std::optional<uint8_t> parsed = parseByte(text, 7);
        if (!parsed)
        {
            return std::nullopt;
        }
        alpha = *parsed;
    }

    return IRenderer::Color{*red, *green, *blue, alpha};
}

void readIntField(const Json::Value& object, const char* field, int& target, const std::filesystem::path& path,
                  int minimum)
{
    const Json::Value& value = object[field];
    if (value.isNull())
    {
        return;
    }
    if (value.isInt() && value.asInt() >= minimum)
    {
        target = value.asInt();
        return;
    }
    logInvalidConfigField(path, field);
}

void readFloatField(const Json::Value& object, const char* field, float& target, const std::filesystem::path& path,
                    float minimum, float maximum)
{
    const Json::Value& value = object[field];
    if (value.isNull())
    {
        return;
    }
    if (value.isNumeric())
    {
        const float parsed = value.asFloat();
        if (parsed >= minimum && parsed <= maximum)
        {
            target = parsed;
            return;
        }
    }
    logInvalidConfigField(path, field);
}

const Json::Value* objectField(const Json::Value& root, const char* field, const std::filesystem::path& path)
{
    const Json::Value& value = root[field];
    if (value.isNull())
    {
        return nullptr;
    }
    if (!value.isObject())
    {
        logInvalidConfigField(path, field);
        return nullptr;
    }
    return &value;
}

void readFontConfig(const Json::Value& root, const std::filesystem::path& path, Style& style)
{
    const Json::Value* font = objectField(root, "font", path);
    if (font == nullptr)
    {
        return;
    }
    readIntField(*font, "size", style.font_size, path, 1);
    readFloatField(*font, "width_factor", style.text_width_factor, path, 0.0F, 10.0F);
}

void readLayoutConfig(const Json::Value& root, const std::filesystem::path& path, Style& style)
{
    const Json::Value* layout = objectField(root, "layout", path);
    if (layout == nullptr)
    {
        return;
    }
    readFloatField(*layout, "padding", style.padding, path, 0.0F, 10'000.0F);
    readFloatField(*layout, "header_height", style.header_height, path, 0.0F, 10'000.0F);
    readFloatField(*layout, "accent_line_height", style.accent_line_height, path, 0.0F, 10'000.0F);
    readFloatField(*layout, "close_button_size", style.close_button_size, path, 0.0F, 10'000.0F);
    readFloatField(*layout, "menu_item_width", style.menu_item_width, path, 0.0F, 10'000.0F);
    readFloatField(*layout, "menu_item_height", style.menu_item_height, path, 0.0F, 10'000.0F);
    readFloatField(*layout, "pool_gap", style.pool_gap, path, 0.0F, 10'000.0F);
    readFloatField(*layout, "pool_drag_edge", style.pool_drag_edge, path, 0.0F, 10'000.0F);
}

void readPaletteConfig(const Json::Value& root, const std::filesystem::path& path, PaletteStyle& palette)
{
    const Json::Value* value = objectField(root, "palette", path);
    if (value == nullptr)
    {
        return;
    }
    readFloatField(*value, "lightness_threshold", palette.lightness_threshold, path, 0.0F, 1.0F);
    readFloatField(*value, "secondary_tint", palette.secondary_tint, path, 0.0F, 1.0F);
    readFloatField(*value, "canvas_tint", palette.canvas_tint, path, 0.0F, 1.0F);
    readFloatField(*value, "hover_mix", palette.hover_mix, path, 0.0F, 1.0F);
    readFloatField(*value, "pressed_mix", palette.pressed_mix, path, 0.0F, 1.0F);
    readFloatField(*value, "border_mix", palette.border_mix, path, 0.0F, 1.0F);
    readFloatField(*value, "dark_surface_lift", palette.dark_surface_lift, path, 0.0F, 1.0F);
    readFloatField(*value, "light_surface_lift", palette.light_surface_lift, path, 0.0F, 1.0F);
}

void readThemesConfig(const Json::Value& root, const std::filesystem::path& path,
                      std::vector<ThemeDescription>& themes)
{
    const Json::Value& value = root["themes"];
    if (value.isNull())
    {
        return;
    }
    if (!value.isArray())
    {
        logInvalidConfigField(path, "themes");
        return;
    }

    std::vector<ThemeDescription> parsed;
    for (const Json::Value& item : value)
    {
        if (!item.isObject() || !item["name"].isString() || item["name"].asString().empty())
        {
            ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "UI config '%s': invalid theme entry, skipped",
                                 path.string().c_str());
            continue;
        }

        const std::string name = item["name"].asString();

        const std::optional<IRenderer::Color> primary   = parseColor(item["primary"]);
        const std::optional<IRenderer::Color> secondary = parseColor(item["secondary"]);
        const std::optional<IRenderer::Color> neutral   = parseColor(item["neutral"]);
        const std::optional<IRenderer::Color> accent    = parseColor(item["accent"]);
        const std::optional<IRenderer::Color> unical    = parseColor(item["unical"]);
        if (!primary || !secondary || !neutral || !accent || !unical)
        {
            ApplicationCore::log(ApplicationCore::LogLevel::WARNING,
                                 "UI config '%s': theme '%s' has invalid colors, skipped", path.string().c_str(),
                                 name.c_str());
            continue;
        }

        parsed.push_back(ThemeDescription{name, *primary, *secondary, *neutral, *accent, *unical});
    }

    if (parsed.empty())
    {
        ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "UI config '%s': no valid themes, using defaults",
                             path.string().c_str());
        return;
    }

    themes = std::move(parsed);
}

void readFontsConfig(const Json::Value& root, const std::filesystem::path& path, std::vector<std::string>& fonts)
{
    const Json::Value& value = root["fonts"];
    if (value.isNull())
    {
        return;
    }
    if (!value.isArray())
    {
        logInvalidConfigField(path, "fonts");
        return;
    }

    std::vector<std::string> parsed;
    for (const Json::Value& item : value)
    {
        if (!item.isString() || item.asString().empty())
        {
            ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "UI config '%s': invalid font entry, skipped",
                                 path.string().c_str());
            continue;
        }
        parsed.push_back(item.asString());
    }

    if (parsed.empty())
    {
        ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "UI config '%s': no valid fonts, using defaults",
                             path.string().c_str());
        return;
    }

    fonts = std::move(parsed);
}

Settings loadSettings(const std::optional<std::filesystem::path>& path)
{
    Settings settings = defaultSettings();

    if (!path)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "UI config location is not resolved, using defaults");
        return settings;
    }

    std::ifstream file(*path);
    if (!file.is_open())
    {
        ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "UI config '%s' not found, using defaults",
                             path->string().c_str());
        return settings;
    }

    Json::CharReaderBuilder builder;
    Json::Value             root;
    std::string             errors;

    if (!Json::parseFromStream(builder, file, &root, &errors) || !root.isObject())
    {
        ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "UI config '%s' is invalid (%s), using defaults",
                             path->string().c_str(), errors.empty() ? "root is not an object" : errors.c_str());
        return settings;
    }

    const Json::Value& default_theme = root["default_theme"];
    if (default_theme.isString() && !default_theme.asString().empty())
    {
        settings.default_theme = default_theme.asString();
    }
    else if (!default_theme.isNull())
    {
        logInvalidConfigField(*path, "default_theme");
    }

    readFontsConfig(root, *path, settings.fonts);
    readThemesConfig(root, *path, settings.themes);
    readFontConfig(root, *path, settings.style);
    readLayoutConfig(root, *path, settings.style);
    readPaletteConfig(root, *path, settings.style.palette);

    return settings;
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
                                               .font_size       = context.style->font_size,
                                               .overflow_action = IRenderer::TextOverflowAction::TAILREPLACE,
                                               .h_align         = IRenderer::TextHAlign::LEFT,
                                               .w_align         = IRenderer::TextWAlign::CENTER,
                                               .padding         = context.style->padding,
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

float contentPadding(const BoxF& box, const Style& style)
{
    const float side = std::min(box.size().x(), box.size().y());
    return std::min(style.padding,
                    std::max(0.0F, (side - static_cast<float>(style.font_size) - style.padding) / 2.0F));
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
                                               .font_size       = context.style->font_size,
                                               .overflow_action = IRenderer::TextOverflowAction::TAILREPLACE,
                                               .h_align         = IRenderer::TextHAlign::CENTER,
                                               .w_align         = IRenderer::TextWAlign::CENTER,
                                               .padding         = contentPadding(box, *context.style),
                                               .line_spacing    = 0.0F,
                                               .wrap            = false});
}

void drawControlContent(IRenderer* renderer, const RenderContext& context,
                        const std::optional<IRenderer::TextureID>& icon, const std::optional<std::string>& text,
                        const IRenderer::Color& color, const BoxF& box)
{
    const BoxF inner = insetBox(box, contentPadding(box, *context.style));

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

BoxF layoutPool(std::list<std::unique_ptr<TObject>>& children, const BoxF& bounds, IUIFramework::Direction direction,
                float gap)
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
        total += gap * static_cast<float>(main_sizes.size() - 1);
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
        offset += main + gap;
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
    setBox(fitToText(bounds, text_, icon_.has_value(), *context_.style));
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
    setBox(fitToText(bounds, text_, icon_.has_value(), *context_.style));
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
    setBox(layoutPool(children(), box(), direction_, context_.style->pool_gap));
}

inline bool SwitcherPoolObject::processEvent(const IInputPump::Event& event)
{
    if (processDrag(*this, drag_, event))
    {
        return true;
    }

    relayout();

    const BoxF bounds = globalBounds(*this);
    if (startDrag(*this, drag_, event, bounds, context_.style->pool_drag_edge))
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
    setBox(layoutPool(children(), box(), direction_, context_.style->pool_gap));
}

inline bool ButtonPoolObject::processEvent(const IInputPump::Event& event)
{
    if (processDrag(*this, drag_, event))
    {
        return true;
    }

    relayout();

    const BoxF bounds = globalBounds(*this);
    if (startDrag(*this, drag_, event, bounds, context_.style->pool_drag_edge))
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
    float item_width  = context_.style->menu_item_width;
    float item_height = context_.style->menu_item_height;
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
    Vec2F                                     close_size_;
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

        const float close_side = context_.style->close_button_size;
        auto        close      = std::make_unique<ButtonObject>(context_, BoxF(Vec2F(close_side, close_side)),
                                                                std::optional<std::string>("X"), std::nullopt, role_,
                                                                std::move(close_action));
        close_size_   = close->box().size();
        close_button_ = close.get();
        addChild(std::move(close));
    }

    const float close_width = close_button_ != nullptr ? close_size_.x() : 0.0F;
    setBox(fitWindowBox(bounds, label_, close_width, *context_.style));
    relayout();
}

inline void WindowMenuObject::relayout()
{
    if (close_button_ == nullptr)
    {
        return;
    }

    const Vec2F size      = box().size();
    const Vec2F close_pos = Vec2F(size.x() - close_size_.x() - context_.style->padding,
                                  (context_.style->header_height - close_size_.y()) / 2.0F);
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
    const BoxF header = makeBox(window.topLeft(), Vec2F(window.size().x(), context_.style->header_height));
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

    const Palette palette = context_.theme->palette(role_);
    const BoxF    bounds  = globalBounds(*this);
    const Vec2F   origin  = bounds.topLeft();
    const float   header_height = context_.style->header_height;
    const BoxF    header_box    = makeBox(origin, Vec2F(bounds.size().x(), header_height));

    drawPanel(renderer, palette.canvas, palette.border, bounds);
    drawPanel(renderer, palette.base, palette.border, header_box);
    renderer->renderRect(context_.theme->accent,
                         makeBox(Vec2F(origin.x(), origin.y() + header_height - context_.style->accent_line_height),
                                 Vec2F(bounds.size().x(), context_.style->accent_line_height)),
                         NO_ANGLE);

    if (label_)
    {
        const float reserved  = close_button_ != nullptr ? close_size_.x() + 2.0F * context_.style->padding : 0.0F;
        const BoxF  label_box = makeBox(header_box.topLeft(), Vec2F(header_box.size().x() - reserved, header_height));
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
    settings_      = loadSettings(configFilePath());
    context_.style = &settings_.style;

    for (const ThemeDescription& theme : settings_.themes)
    {
        registryTheme(theme.name, theme.primary, theme.secondary, theme.neutral, theme.accent, theme.unical);
    }

    if (!setTheme(settings_.default_theme))
    {
        ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "UI config: theme '%s' is not registered",
                             settings_.default_theme.c_str());
        if (!settings_.themes.empty())
        {
            setTheme(settings_.themes.front().name);
        }
    }

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
    const ThemeDescription description{name, primary, secondary, neutral, accent, unical};
    themes_.insert_or_assign(name, makeTheme(description, settings_.style.palette));
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

    for (const std::string& font : settings_.fonts)
    {
        const std::optional<IRenderer::FontID> loaded = renderer->loadFont(font);
        if (loaded)
        {
            context_.font = loaded;
            ApplicationCore::log(ApplicationCore::LogLevel::OK, "UI font: %s", font.c_str());
            return;
        }
    }

    ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "No UI font from the UI config is loaded");
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
