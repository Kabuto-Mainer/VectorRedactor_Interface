#pragma once

#include "IPalette.hpp"
#include "IUIFramework.hpp"
#include "Interfaces/Package.hpp"
#include "Utils/TWidget.hpp"

#include <cstddef>
#include <optional>
#include <vector>

extern "C" InitPackageFunc INIT_PACKAGE_FUNC_NAME;

namespace Kabuto
{

namespace Palette
{

class CPalette final : public TWidget, public IPalette
{
public:
    explicit CPalette(IUIFramework& framework);
    ~CPalette() override = default;

    CPalette(const CPalette&)            = delete;
    CPalette(CPalette&&)                 = delete;
    CPalette& operator=(const CPalette&) = delete;
    CPalette& operator=(CPalette&&)      = delete;

    Color color() const override;
    void  setColor(const Color& new_color) override;

    BoxF box() const override;
    void setBox(const BoxF& new_box) override;

    bool processEvent(const IInputPump::Event& event) override;
    void render() override;

private:
    struct Swatch
    {
        IUIFramework::Switcher* switcher    = nullptr;
        std::size_t             color_index = 0;
    }; // struct Swatch

    void buildSwitchers(IUIFramework& framework);
    void layoutWidgets();

    BoxF                       globalBox() const;
    std::optional<std::size_t> swatchAt(const Vec2F& global_point) const;
    std::optional<std::size_t> matchingSwatch() const;
    void                       selectSwatch(std::size_t index);

    IUIFramework::SwitcherPool* pool_ = nullptr;
    std::vector<Swatch>         swatches_;
    Color                       color_;
    std::optional<std::size_t>  hovered_;
    bool                        pressed_ = false;
}; // class CPalette

} // namespace Palette

} // namespace Kabuto
