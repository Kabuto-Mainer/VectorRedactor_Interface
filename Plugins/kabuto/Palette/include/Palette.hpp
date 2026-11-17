#pragma once

#include "IPalette.hpp"
#include "IUIFramework.hpp"
#include "Interfaces/Package.hpp"

#include <memory>

extern "C" InitPackageFunc INIT_PACKAGE_FUNC_NAME;

namespace Kabuto
{

namespace Palette
{

/**
 * IPalette implementation.
 *
 * The service is deliberately not a tree object: it only stores the selected
 * color. The tree object is a UIFramework SwitcherPool created by
 * createPaletteView().
 */
class CPalette final : public IPalette
{
public:
    CPalette();
    ~CPalette() override = default;

    CPalette(const CPalette&)            = delete;
    CPalette(CPalette&&)                 = delete;
    CPalette& operator=(const CPalette&) = delete;
    CPalette& operator=(CPalette&&)      = delete;

    Color color() const override;
    void  setColor(const Color& new_color) override;

private:
    Color color_;
}; // class CPalette

/**
 * Builds the palette tree object.
 *
 * Switchers are supplied by the UI framework and report selection through
 * IPalette, so the returned pool contains no palette-specific view class.
 */
std::unique_ptr<IUIFramework::SwitcherPool> createPaletteView(IUIFramework& framework, IPalette& palette);

} // namespace Palette

} // namespace Kabuto
