#include "Palette.hpp"

#include "Interfaces/IDesktop.hpp"

#include <array>
#include <variant>

namespace Yasha
{
namespace
{
constexpr std::array COLORS{
    IRenderer::Color::BLACK,       IRenderer::Color::WHITE,      IRenderer::Color::RED,
    IRenderer::Color::GREEN,       IRenderer::Color::BLUE,       IRenderer::Color::YELLOW,
    IRenderer::Color::CYAN,        IRenderer::Color::MAGENTA,    IRenderer::Color::ORANGE,
    IRenderer::Color::PURPLE,      IRenderer::Color::PINK,       IRenderer::Color::BROWN,
    IRenderer::Color::GRAY,        IRenderer::Color::LIGHT_GRAY, IRenderer::Color::DARK_GRAY,
    IRenderer::Color::TRANSPARENT,
};
} // namespace

class Palette::SelectColorCommand final : public ACommand
{
public:
    SelectColorCommand(Palette& palette, std::size_t index) : palette_(palette), index_(index) {}

    void execute() const override { palette_.selectColor(index_); }

private:
    Palette&    palette_;
    std::size_t index_;
};

class Palette::SwatchButton final : public UI::ColorButton
{
public:
    SwatchButton(const BoxF& box, ACommand& command, IRenderer::Color color) :
        ColorButton(box, command, color)
    {
    }

    bool processEvent(const IInputPump::Event& event) override
    {
        const auto* mouse = std::get_if<IInputPump::EventMouse>(&event);
        if (mouse == nullptr || mouse->type() != IInputPump::EventMouse::Type::LEFT_BUTTON_PRESSING ||
            !globalBox().contains(mouse->cursorPos()))
        {
            return false;
        }
        command().execute();
        return true;
    }
};

Palette::Palette(const BoxF& window, float margin) : pool_(Vec2F{}, UI::ButtonPoolDirection::VERTICAL)
{
    const Vec2F position{window.right() - margin - UI::BUTTON_SIZE, window.top() + margin};
    setBox(BoxF(position, position + Vec2F{UI::BUTTON_SIZE, UI::BUTTON_SIZE * COLORS.size()}));
    pool_.setParent(this);
    commands_.reserve(COLORS.size());
    
    for (std::size_t index = 0; index < COLORS.size(); ++index)
    {
        commands_.push_back(std::make_unique<SelectColorCommand>(*this, index));
        pool_.add(std::make_unique<SwatchButton>(BoxF(Vec2F{UI::BUTTON_SIZE}), *commands_.back(), COLORS[index]));
    }

    selectColor(0);
}

bool Palette::processEvent(const IInputPump::Event& event)
{
    return pool_.processEvent(event);
}

void Palette::render()
{
    pool_.render();
}

IRenderer::Color Palette::selectedColor() const
{
    return COLORS[selected_index_];
}

void Palette::selectColor(std::size_t index)
{
    selected_index_ = index;
    std::size_t button_index = 0;
    for (const auto& child : pool_.buttons())
    {
        static_cast<UI::Button&>(*child).setState(button_index == index ? UI::ButtonState::PRESSED : UI::ButtonState::NO_PRESSED);
        ++button_index;
    }
}

ApplicationCore::Error initializePalette(const BoxF& window, float margin)
{
    auto& core = ApplicationCore::instance();
    void* existing = nullptr;
    if (core.queryInterface(IID_PALETTE, &existing) == ApplicationCore::Error::SUCCESS)
    {
        return ApplicationCore::Error::INTERFACE_ALREADY_EXISTS;
    }

    IDesktop* desktop = nullptr;
    if (core.queryInterface(IID_DESKTOP, reinterpret_cast<void**>(&desktop)) != ApplicationCore::Error::SUCCESS ||
        desktop == nullptr)
    {
        return ApplicationCore::Error::INTERFACE_NOT_FOUND;
    }

    auto palette = std::make_unique<Palette>(window, margin);
    auto* interface = static_cast<IPalette*>(palette.get());
    const auto id = palette->id();
    if (desktop->add(std::move(palette)) != IDesktop::Error::SUCCESS)
    {
        return ApplicationCore::Error::INVALID_ARGS;
    }

    const auto error = core.registerInterface(IID_PALETTE, interface);
    if (error != ApplicationCore::Error::SUCCESS)
    {
        desktop->remove(id);
    }
    return error;
}

} // namespace UI
