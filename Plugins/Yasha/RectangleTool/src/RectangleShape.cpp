#include "RectangleShape.hpp"

#include "Core/ApplicationCore.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace Yasha::Tools
{

RectangleShape::RectangleShape(const Vec2F& size)
{
    setScaling(size / 2.0F);
}

bool RectangleShape::processEvent(const IInputPump::Event& /*event*/)
{
    return false;
}

void RectangleShape::render()
{
    IRenderer* renderer = nullptr;
    if (ApplicationCore::instance().queryInterface(IID_RENDERER, reinterpret_cast<void**>(&renderer)) !=
            ApplicationCore::Error::SUCCESS ||
        renderer == nullptr)
    {
        return;
    }

    const Vec2F half_size{std::abs(scaling().x()), std::abs(scaling().y())};
    const Vec2F size = half_size * 2.0F;
    const float angle = rotation();
    const Vec2F rotated_half{std::cos(angle) * half_size.x() - std::sin(angle) * half_size.y(),
                             std::sin(angle) * half_size.x() + std::cos(angle) * half_size.y()};
    const Vec2F top_left = mapLocalToGlobal(Vec2F{}) - rotated_half;
    const BoxF box{top_left, top_left + size};
    const float degrees = angle * (180.0F / std::numbers::pi_v<float>);
    renderer->renderRect(color(), box, degrees);
    renderer->renderRectLines(IRenderer::Color::BLACK, box, degrees);
}

bool RectangleShape::hitTest(const Vec2F& pos) const
{
    if (std::abs(scaling().x()) < 1.0e-6F || std::abs(scaling().y()) < 1.0e-6F)
    {
        return false;
    }
    return BoxF{-1.0F, -1.0F, 1.0F, 1.0F}.contains(mapGlobalToLocal(pos));
}

BoxF RectangleShape::axesAlignedBoundingBox() const
{
    const std::array<Vec2F, 4> corners = {
        transform().transformPoint(Vec2F{-1.0F, -1.0F}),
        transform().transformPoint(Vec2F{-1.0F, 1.0F}),
        transform().transformPoint(Vec2F{1.0F, -1.0F}),
        transform().transformPoint(Vec2F{1.0F, 1.0F}),
    };
    float left = corners[0].x();
    float right = corners[0].x();
    float top = corners[0].y();
    float bottom = corners[0].y();
    for (const Vec2F& corner : corners)
    {
        left = std::min(left, corner.x());
        right = std::max(right, corner.x());
        top = std::min(top, corner.y());
        bottom = std::max(bottom, corner.y());
    }
    return BoxF{left, top, right, bottom};
}

IDocument::TShape::TypeID RectangleShape::tid() const
{
    return TYPE_ID;
}

IDocument::TShape::Error RectangleShape::queryComponent(IDocument::AShapeComponent::ID coid,
                                                          IDocument::AShapeComponent** component)
{
    if (component == nullptr)
    {
        return Error::INVALID_ARGS;
    }
    *component = nullptr;
    if (coid == ColorComponent::COID)
    {
        *component = static_cast<ColorComponent*>(this);
        return Error::SUCCESS;
    }
    return Error::COMPONENT_NOT_FOUND;
}

IDocument::TShape::Error RectangleShape::queryComponent(IDocument::AShapeComponent::ID coid,
                                                          const IDocument::AShapeComponent** component) const
{
    if (component == nullptr)
    {
        return Error::INVALID_ARGS;
    }
    *component = nullptr;
    if (coid == ColorComponent::COID)
    {
        *component = static_cast<const ColorComponent*>(this);
        return Error::SUCCESS;
    }
    return Error::COMPONENT_NOT_FOUND;
}

} // namespace Yasha::Tools
