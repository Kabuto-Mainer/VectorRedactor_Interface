#pragma once

#include "Interfaces/Package.hpp"
#include "export/IPalette.hpp"
#include "kabuto/UIFW/export/IUIFramework.hpp"

#include <memory>
#include <vector>

extern "C" InitPackageFunc INIT_PACKAGE_FUNC_NAME;

namespace Kabuto
{

namespace Palette
{

inline constexpr IRenderer::Color DEFAULT_COLOR{0, 122, 255, 255};

std::vector<IRenderer::Color> defaultSwatches();

struct PaletteConfig
{
    float                         left            = 24.0F;
    float                         top             = 24.0F;
    float                         width           = 264.0F;
    float                         switcher_height = 28.0F;
    IRenderer::Color              default_color   = DEFAULT_COLOR;
    std::vector<IRenderer::Color> swatches        = defaultSwatches();
};

class CPalette final : public IPalette
{
public:
    CPalette(const CPalette&)            = delete;
    CPalette(CPalette&&)                 = delete;
    CPalette& operator=(const CPalette&) = delete;
    CPalette& operator=(CPalette&&)      = delete;

    static CPalette& instance();

    Color color() const override;
    void  setColor(const Color& new_color) override;

private:
    CPalette();
    ~CPalette() override = default;

    Color color_;
}; // class CPalette

std::unique_ptr<IUIFramework::SwitcherPool> createPaletteView(IUIFramework& framework, IPalette& palette,
                                                              const PaletteConfig& config);

} // namespace Palette

} // namespace Kabuto
