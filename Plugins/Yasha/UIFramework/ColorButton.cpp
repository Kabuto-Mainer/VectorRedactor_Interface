#include "UIFramework.hpp"

namespace UI
{

ColorButton::ColorButton(const Vec2F& pos, const Vec2F& size, ACommand& command, IRenderer::Color color) :
    Button(pos, size, command, color)
{
}

ColorButton::ColorButton(const BoxF& box, ACommand& command, IRenderer::Color color) :
    Button(box, command, color)
{
}

void ColorButton::render()
{
    Button::render();
    if (IRenderer* target = renderer())
    {
        target->renderRectLines(IRenderer::Color{0U, 82U, 172U, 255U}, globalBox());
    }
}

} // namespace UI
