#include "include/Palette.hpp"

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDesktop.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Utils/Vec2F.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <json/json.h>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Kabuto
{

namespace Palette
{

namespace
{

constexpr uint8_t OPAQUE_ALPHA = 255;

constexpr const char* PALETTE_CONFIG_FILENAME = "Palette.json";

constexpr std::size_t COLOR_CHANNEL_COUNT = 3;
constexpr std::size_t HEX_COLOR_LENGTH    = 1 + 2 * COLOR_CHANNEL_COUNT;

constexpr IRenderer::Color opaque(uint8_t red, uint8_t green, uint8_t blue)
{
    return IRenderer::Color{red, green, blue, OPAQUE_ALPHA};
}

std::string colorLabel(const IRenderer::Color& color)
{
    char buffer[8];
    std::snprintf(buffer, sizeof(buffer), "#%02X%02X%02X", color.r, color.g, color.b);
    return buffer;
}

int hexDigit(char symbol)
{
    if (symbol >= '0' && symbol <= '9')
    {
        return symbol - '0';
    }
    if (symbol >= 'a' && symbol <= 'f')
    {
        return symbol - 'a' + 10;
    }
    if (symbol >= 'A' && symbol <= 'F')
    {
        return symbol - 'A' + 10;
    }

    return -1;
}

std::optional<IRenderer::Color> parseColor(std::string_view text)
{
    if (text.size() != HEX_COLOR_LENGTH || text.front() != '#')
    {
        return std::nullopt;
    }

    uint8_t channels[COLOR_CHANNEL_COUNT] = {};
    for (std::size_t i = 0; i < COLOR_CHANNEL_COUNT; ++i)
    {
        const int high = hexDigit(text[1 + 2 * i]);
        const int low  = hexDigit(text[2 + 2 * i]);
        if (high < 0 || low < 0)
        {
            return std::nullopt;
        }

        channels[i] = static_cast<uint8_t>(high * 16 + low);
    }

    return IRenderer::Color{channels[0], channels[1], channels[2], OPAQUE_ALPHA};
}

std::filesystem::path executableDirectory()
{
    std::error_code             error;
    const std::filesystem::path self = std::filesystem::read_symlink("/proc/self/exe", error);
    if (!error && !self.empty())
    {
        return self.parent_path();
    }

    return std::filesystem::current_path(error);
}

std::filesystem::path configFilePath()
{
    return executableDirectory() / "config" / PALETTE_CONFIG_FILENAME;
}

void logInvalidConfigField(const std::filesystem::path& path, const char* field)
{
    ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "Palette config '%s': invalid '%s', using default",
                         path.string().c_str(), field);
}

float readFloat(const Json::Value& root, const char* field, float fallback, bool positive_only,
                const std::filesystem::path& path)
{
    const Json::Value& value = root[field];

    if (value.isNull())
    {
        return fallback;
    }

    if (!value.isNumeric())
    {
        logInvalidConfigField(path, field);
        return fallback;
    }

    constexpr double FLOAT_MAX = static_cast<double>(std::numeric_limits<float>::max());

    const double number = value.asDouble();
    if (!std::isfinite(number) || number > FLOAT_MAX || number < -FLOAT_MAX || (positive_only && number <= 0.0))
    {
        logInvalidConfigField(path, field);
        return fallback;
    }

    return static_cast<float>(number);
}

IRenderer::Color readColor(const Json::Value& root, const char* field, const IRenderer::Color& fallback,
                           const std::filesystem::path& path)
{
    const Json::Value& value = root[field];

    if (value.isNull())
    {
        return fallback;
    }

    if (!value.isString())
    {
        logInvalidConfigField(path, field);
        return fallback;
    }

    const std::optional<IRenderer::Color> color = parseColor(value.asString());
    if (!color)
    {
        logInvalidConfigField(path, field);
        return fallback;
    }

    return *color;
}

std::vector<IRenderer::Color> readSwatches(const Json::Value& root, const std::vector<IRenderer::Color>& fallback,
                                           const std::filesystem::path& path)
{
    const Json::Value& value = root["swatches"];

    if (value.isNull())
    {
        return fallback;
    }

    if (!value.isArray())
    {
        logInvalidConfigField(path, "swatches");
        return fallback;
    }

    std::vector<IRenderer::Color> swatches;
    swatches.reserve(value.size());

    for (Json::ArrayIndex index = 0; index < value.size(); ++index)
    {
        const Json::Value& item = value[index];

        const std::optional<IRenderer::Color> color = item.isString() ? parseColor(item.asString()) : std::nullopt;
        if (!color)
        {
            ApplicationCore::log(ApplicationCore::LogLevel::WARNING,
                                 "Palette config '%s': invalid 'swatches[%u]', skipped", path.string().c_str(),
                                 static_cast<unsigned>(index));
            continue;
        }

        swatches.push_back(*color);
    }

    if (swatches.empty())
    {
        ApplicationCore::log(ApplicationCore::LogLevel::WARNING,
                             "Palette config '%s': 'swatches' is empty, using defaults", path.string().c_str());
        return fallback;
    }

    return swatches;
}

PaletteConfig loadPaletteConfig(const std::filesystem::path& path)
{
    PaletteConfig config;

    std::ifstream file(path);
    if (!file.is_open())
    {
        ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "Palette config '%s' not found, using defaults",
                             path.string().c_str());
        return config;
    }

    Json::CharReaderBuilder builder;
    Json::Value             root;
    std::string             errors;

    if (!Json::parseFromStream(builder, file, &root, &errors) || !root.isObject())
    {
        ApplicationCore::log(ApplicationCore::LogLevel::WARNING, "Palette config '%s' is invalid (%s), using defaults",
                             path.string().c_str(), errors.empty() ? "root is not an object" : errors.c_str());
        return config;
    }

    config.left            = readFloat(root, "left", config.left, false, path);
    config.top             = readFloat(root, "top", config.top, false, path);
    config.width           = readFloat(root, "width", config.width, true, path);
    config.switcher_height = readFloat(root, "switcher_height", config.switcher_height, true, path);
    config.default_color   = readColor(root, "default_color", config.default_color, path);
    config.swatches        = readSwatches(root, config.swatches, path);

    return config;
}

} // namespace

std::vector<IRenderer::Color> defaultSwatches()
{
    return {
        opaque(0, 0, 0),       opaque(32, 32, 32),    opaque(64, 64, 64),    opaque(96, 96, 96),
        opaque(128, 128, 128), opaque(160, 160, 160), opaque(208, 208, 208), opaque(255, 255, 255),

        opaque(235, 64, 52),   opaque(255, 145, 0),   opaque(255, 214, 0),   opaque(52, 199, 89),
        opaque(0, 199, 190),   opaque(0, 122, 255),   opaque(88, 86, 214),   opaque(175, 82, 222),

        opaque(139, 0, 0),     opaque(139, 69, 19),   opaque(128, 128, 0),   opaque(34, 139, 34),
        opaque(0, 128, 128),   opaque(0, 0, 128),     opaque(75, 0, 130),    opaque(128, 0, 128),

        opaque(255, 192, 203), opaque(255, 218, 185), opaque(255, 255, 153), opaque(144, 238, 144),
        opaque(127, 255, 212), opaque(135, 206, 250), opaque(216, 191, 216), opaque(211, 211, 211),
    };
}

CPalette& CPalette::instance()
{
    static CPalette palette;
    return palette;
}

CPalette::CPalette() : color_(DEFAULT_COLOR)
{
}

IPalette::Color CPalette::color() const
{
    return color_;
}

void CPalette::setColor(const Color& new_color)
{
    color_ = new_color;
}

std::unique_ptr<IUIFramework::SwitcherPool> createPaletteView(IUIFramework& framework, IPalette& palette,
                                                              const PaletteConfig& config)
{
    IUIFramework::SwitcherPoolDescription pool_description;
    pool_description.min_point = Vec2F(config.left, config.top);
    pool_description.size = Vec2F(config.width, config.switcher_height * static_cast<float>(config.swatches.size()));
    pool_description.direction = IUIFramework::Direction::VERTICAL;
    pool_description.role      = IUIFramework::Role::DEFAULT;

    std::unique_ptr<IUIFramework::SwitcherPool> pool = framework.create(pool_description);
    if (!pool)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Palette can't create a switcher pool");
        return nullptr;
    }

    for (const IRenderer::Color& color : config.swatches)
    {
        IUIFramework::SwitcherDescription description;
        description.box  = BoxF(Vec2F(config.width, config.switcher_height));
        description.text = colorLabel(color);
        description.role = IUIFramework::Role::DEFAULT;
        description.on_active.callbacks.emplace_back([&palette, color] { palette.setColor(color); });

        std::unique_ptr<IUIFramework::Switcher> switcher = framework.create(description);
        if (!switcher)
        {
            ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Palette can't create a switcher");
            break;
        }

        pool->add(std::move(switcher));
    }

    return pool;
}

} // namespace Palette

} // namespace Kabuto



extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    ApplicationCore& core = ApplicationCore::instance();

    Kabuto::IUIFramework* framework = nullptr;
    if (core.queryInterface(Kabuto::IID_IUIFramework, reinterpret_cast<void**>(&framework)) !=
            ApplicationCore::Error::SUCCESS ||
        framework == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Palette can't find the UI framework");
        return false;
    }

    IDesktop* desktop = nullptr;
    if (core.queryInterface(IID_DESKTOP, reinterpret_cast<void**>(&desktop)) != ApplicationCore::Error::SUCCESS ||
        desktop == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Palette can't find the desktop");
        return false;
    }

    Kabuto::Palette::CPalette& palette = Kabuto::Palette::CPalette::instance();

    const Kabuto::Palette::PaletteConfig config = Kabuto::Palette::loadPaletteConfig(Kabuto::Palette::configFilePath());
    palette.setColor(config.default_color);

    std::unique_ptr<Kabuto::IUIFramework::SwitcherPool> view =
        Kabuto::Palette::createPaletteView(*framework, palette, config);
    if (!view)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Palette can't create its view");
        return false;
    }

    const TObject::ID view_id = view->id();
    if (desktop->add(std::move(view)) != IDesktop::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Palette can't be added to the desktop");
        return false;
    }

    const ApplicationCore::Error error =
        core.registerInterface(Kabuto::IID_PALETTE, static_cast<void*>(static_cast<Kabuto::IPalette*>(&palette)));
    if (error != ApplicationCore::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Failed to register IPalette: %s",
                             ApplicationCore::strError(error));
        desktop->remove(view_id);
        return false;
    }

    return true;
}
