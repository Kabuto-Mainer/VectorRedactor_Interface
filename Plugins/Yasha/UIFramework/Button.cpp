#include "Core/ApplicationCore.hpp"
#include "UIFramework.hpp"

#include <cstdint>
#include <utility>
#include <variant>

namespace UI
{

Button::Button(const Vec2F& pos, const Vec2F& size, ACommand& command, IRenderer::Color color, ButtonState state) :
    command_(command), color_(color), state_(state)
{
    setBox(BoxF(pos, pos + size));
}

bool Button::processEvent(const IInputPump::Event& event)
{
    const auto* mouse = std::get_if<IInputPump::EventMouse>(&event);
    if (mouse == nullptr ||
        mouse->type() != IInputPump::EventMouse::Type::LEFT_BUTTON_PRESSING ||
        !globalBox().contains(mouse->cursorPos()))
    {
        return false;
    }

    state_ = state_ == ButtonState::PRESSED ? ButtonState::NO_PRESSED : ButtonState::PRESSED;
    if (state_ == ButtonState::PRESSED)
    {
        command_.execute();
    }
    return true;
}

void Button::render()
{
    IRenderer* target = renderer();
    if (target == nullptr)
    {
        return;
    }

    IRenderer::Color fill = color_;
    if (state_ == ButtonState::PRESSED)
    {
        fill.r = static_cast<uint8_t>(fill.r * 0.7F);
        fill.g = static_cast<uint8_t>(fill.g * 0.7F);
        fill.b = static_cast<uint8_t>(fill.b * 0.7F);
    }
    target->renderRect(fill, globalBox());
    target->renderRectLines(IRenderer::Color::BLACK, globalBox());
}

BoxF Button::globalBox() const
{
    Vec2F          top_left{};
    const TObject* object = this;
    while (object != nullptr)
    {
        if (const auto* widget = dynamic_cast<const TWidget*>(object))
        {
            top_left = top_left + widget->box().topLeft();
        }
        object = object->parent();
    }
    return BoxF(top_left, top_left + box().size());
}

IRenderer* Button::renderer() const
{
    IRenderer* result = nullptr;
    if (ApplicationCore::instance().queryInterface(IID_RENDERER, reinterpret_cast<void**>(&result)) !=
        ApplicationCore::Error::SUCCESS)
    {
        return nullptr;
    }
    return result;
}

ButtonText::ButtonText(const Vec2F& pos, const Vec2F& size, std::string name, ACommand& command, IRenderer::FontID font,
                       IRenderer::Color color) : Button(pos, size, command, color), name_(std::move(name)), font_(font)
{
}

void ButtonText::render()
{
    Button::render();
    if (IRenderer* target = renderer())
    {
        IRenderer::TextFormat format{};
        format.font      = font_;
        format.font_size = 15;
        format.h_align   = IRenderer::TextHAlign::CENTER;
        format.w_align   = IRenderer::TextWAlign::CENTER;
        target->renderText(IRenderer::Color::WHITE, globalBox(), name_, format);
    }
}

} // namespace UI
