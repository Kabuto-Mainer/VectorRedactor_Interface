#pragma once

#include "Core/ApplicationCore.hpp"
#include "UIFramework/UIFramework.hpp"

#include <cstddef>
#include <memory>
#include <vector>

namespace Yasha
{

inline constexpr IID IID_PALETTE = 6;

class IPalette
{
public:
    virtual ~IPalette() = default;
    virtual IRenderer::Color selectedColor() const = 0;
};

class Palette final : public TWidget, public IPalette
{
public:
    explicit Palette(const BoxF& window, float margin = 16.0F);

    bool processEvent(const IInputPump::Event& event) override;
    void render() override;

    IRenderer::Color selectedColor() const override;

private:
    class SelectColorCommand;
    class SwatchButton;

    void selectColor(std::size_t index);

    std::vector<std::unique_ptr<ACommand>> commands_;
    UI::ButtonPool                             pool_;
    std::size_t                            selected_index_ = 0;
};

ApplicationCore::Error initializePalette(const BoxF& window, float margin = 16.0F);

} // namespace UI
