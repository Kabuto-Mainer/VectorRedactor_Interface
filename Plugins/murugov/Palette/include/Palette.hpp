#pragma once

#include "Core/ApplicationCore.hpp"
#include "IMurugovPalette.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/Package.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/TWidget.hpp"
#include "Utils/Vec2F.hpp"

#include <array>
#include <string>
#include <cstddef>

extern "C" InitPackageFunc INIT_PACKAGE_FUNC_NAME;

namespace Murugov
{

class TPaletteWidget final : public TWidget
{
public:
    static constexpr std::array<IRenderer::Color, 16> COLORS = {
        IRenderer::Color::WHITE,      IRenderer::Color::BLACK,
        IRenderer::Color::LIGHT_GRAY, IRenderer::Color::DARK_GRAY,
        IRenderer::Color::RED,        IRenderer::Color::ORANGE,
        IRenderer::Color::YELLOW,     IRenderer::Color::BROWN,
        IRenderer::Color::GREEN,      IRenderer::Color::CYAN,
        IRenderer::Color::BLUE,       IRenderer::Color::PURPLE,
        IRenderer::Color::PINK,       IRenderer::Color::MAGENTA,
        IRenderer::Color::GRAY,       IRenderer::Color::TRANSPARENT,
    };

    static constexpr float SWATCH_SIZE = 32.0F;
    static constexpr float GAP         = 6.0F;
    static constexpr float PADDING     = 8.0F;
    static constexpr int   COLUMNS     = 1;

    TPaletteWidget(const Vec2F& top_right_anchor, IRenderer& renderer);

    bool processEvent(const IInputPump::Event& event) override;
    void render() override;

    IRenderer::Color selectedColor() const { return COLORS[selected_index_]; }

private:
    BoxF swatchBox(std::size_t index) const;

    IRenderer&  renderer_;
    std::size_t selected_index_ = 0;
};

class Palette final : public IPalette
{
private:
    bool            initialized_ = false;
    TPaletteWidget* widget_      = nullptr;
    TObject::ID     widget_id_   = 0;

    Palette();
    ~Palette() override;

    struct Config
    {
        float anchor_x = 1600.0F - 16.0F;
        float anchor_y = 40.0F;
    };

    static Config loadConfig(const std::string& filepath);

public:
    Palette(const Palette&)            = delete;
    Palette(Palette&&)                 = delete;
    Palette& operator=(const Palette&) = delete;
    Palette& operator=(Palette&&)      = delete;

    static Palette& instance();

    bool isValid() const { return initialized_; }

    /* from IPalette */
    
    IRenderer::Color selectedColor() const override;
};

} // namespace Murugov