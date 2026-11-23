#include "UIFramework.hpp"

namespace UI
{

IconButton::IconButton(const Vec2F& pos, const Vec2F& size, ACommand& command, IRenderer::TextureID texture,
                       IRenderer::Color color) : Button(pos, size, command, color), texture_(texture)
{
}

IconButton::IconButton(const BoxF& box, ACommand& command, IRenderer::TextureID texture,
                       IRenderer::Color color) : Button(box, command, color), texture_(texture)
{
}

void IconButton::render()
{
    Button::render();
    if (IRenderer* target = renderer())
    {
        target->renderTexture(texture_, globalBox());
    }
}

} // namespace UI
