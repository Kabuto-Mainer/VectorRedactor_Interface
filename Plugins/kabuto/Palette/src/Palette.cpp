#include "Palette.hpp"

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDesktop.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Utils/Vec2F.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <utility>

namespace Kabuto
{

namespace Palette
{

namespace
{

constexpr uint8_t OPAQUE_ALPHA = 255;

constexpr float DEFAULT_LEFT    = 24.0F;
constexpr float DEFAULT_TOP     = 24.0F;
constexpr float DEFAULT_WIDTH   = 264.0F;
constexpr float SWITCHER_HEIGHT = 28.0F;

constexpr IRenderer::Color DEFAULT_COLOR{0, 122, 255, OPAQUE_ALPHA};

constexpr IRenderer::Color opaque(uint8_t red, uint8_t green, uint8_t blue)
{
    return IRenderer::Color{red, green, blue, OPAQUE_ALPHA};
}

constexpr std::size_t SWATCH_COUNT = 32;

constexpr std::array<IRenderer::Color, SWATCH_COUNT> SWATCHES = {
    opaque(0, 0, 0),       opaque(32, 32, 32),    opaque(64, 64, 64),    opaque(96, 96, 96),
    opaque(128, 128, 128), opaque(160, 160, 160), opaque(208, 208, 208), opaque(255, 255, 255),

    opaque(235, 64, 52),   opaque(255, 145, 0),   opaque(255, 214, 0),   opaque(52, 199, 89),
    opaque(0, 199, 190),   opaque(0, 122, 255),   opaque(88, 86, 214),   opaque(175, 82, 222),

    opaque(139, 0, 0),     opaque(139, 69, 19),   opaque(128, 128, 0),   opaque(34, 139, 34),
    opaque(0, 128, 128),   opaque(0, 0, 128),     opaque(75, 0, 130),    opaque(128, 0, 128),

    opaque(255, 192, 203), opaque(255, 218, 185), opaque(255, 255, 153), opaque(144, 238, 144),
    opaque(127, 255, 212), opaque(135, 206, 250), opaque(216, 191, 216), opaque(211, 211, 211),
};

std::string colorLabel(const IRenderer::Color& color)
{
    char buffer[8];
    std::snprintf(buffer, sizeof(buffer), "#%02X%02X%02X", color.r, color.g, color.b);
    return buffer;
}

} // namespace

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

std::unique_ptr<IUIFramework::SwitcherPool> createPaletteView(IUIFramework& framework, IPalette& palette)
{
    IUIFramework::SwitcherPoolDescription pool_description;
    pool_description.min_point = Vec2F(DEFAULT_LEFT, DEFAULT_TOP);
    pool_description.size      = Vec2F(DEFAULT_WIDTH, SWITCHER_HEIGHT * static_cast<float>(SWATCH_COUNT));
    pool_description.direction = IUIFramework::Direction::VERTICAL;
    pool_description.role      = IUIFramework::Role::DEFAULT;

    std::unique_ptr<IUIFramework::SwitcherPool> pool = framework.create(pool_description);
    if (!pool)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Palette can't create a switcher pool");
        return nullptr;
    }

    for (const IRenderer::Color& color : SWATCHES)
    {
        IUIFramework::SwitcherDescription description;
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

    std::unique_ptr<Kabuto::IUIFramework::SwitcherPool> view = Kabuto::Palette::createPaletteView(*framework, palette);
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
