#pragma once

#include "Interfaces/Package.hpp"
#include "export/IPalette.hpp"
#include "export/IUIFramework.hpp"

#include <memory>

extern "C" InitPackageFunc INIT_PACKAGE_FUNC_NAME;

namespace Kabuto
{

namespace Palette
{

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

std::unique_ptr<IUIFramework::SwitcherPool> createPaletteView(IUIFramework& framework, IPalette& palette);

} // namespace Palette

} // namespace Kabuto
