#include "EllipseShape.hpp"

#include "Core/ApplicationCore.hpp"
#include "QueryInterface.hpp"

#include <cmath>
#include <numbers>

namespace Yasha::Tools
{

EllipseShape::EllipseShape(const Vec2F& size)
{
    setScaling(size / 2.0F);
}

bool EllipseShape::processEvent(const IInputPump::Event& /*event*/)
{
    return false;
}

void EllipseShape::render()
{
    IRenderer* renderer = nullptr;
    if (ApplicationCore::instance().queryInterface(IID_RENDERER, reinterpret_cast<void**>(&renderer)) !=
            ApplicationCore::Error::SUCCESS ||
        renderer == nullptr)
    {
        return;
    }

    const Vec2F radius{std::abs(scaling().x()), std::abs(scaling().y())};
    const Vec2F center = mapLocalToGlobal(Vec2F{});
    const BoxF  box{center - radius, center + radius};
    const float degrees = rotation() * (180.0F / std::numbers::pi_v<float>);
    renderer->renderEllipse(color(), box, degrees);
    renderer->renderEllipseLines(IRenderer::Color::BLACK, box, degrees);
}

bool EllipseShape::hitTest(const Vec2F& pos) const
{
    if (std::abs(scaling().x()) < 1.0e-6F || std::abs(scaling().y()) < 1.0e-6F)
    {
        return false;
    }
    return mapGlobalToLocal(pos).len2() <= 1.0F;
}

BoxF EllipseShape::axesAlignedBoundingBox() const
{
    const Vec2F center = mapLocalToGlobal(Vec2F{});
    const Vec2F x_axis = mapLocalToGlobal(Vec2F{1.0F, 0.0F}) - center;
    const Vec2F y_axis = mapLocalToGlobal(Vec2F{0.0F, 1.0F}) - center;
    const Vec2F extent{std::hypot(x_axis.x(), y_axis.x()), std::hypot(x_axis.y(), y_axis.y())};
    return BoxF{center - extent, center + extent};
}

IDocument::TShape::TypeID EllipseShape::tid() const
{
    return TYPE_ID;
}

IDocument::TShape::Error EllipseShape::queryComponent(IDocument::AShapeComponent::ID coid,
                                                      IDocument::AShapeComponent**   component)
{
    if (component == nullptr)
    {
        return Error::INVALID_ARGS;
    }
    *component = nullptr;
    if (coid == EllipseColorComponent::COID)
    {
        *component = static_cast<EllipseColorComponent*>(this);
        return Error::SUCCESS;
    }
    return Error::COMPONENT_NOT_FOUND;
}

IDocument::TShape::Error EllipseShape::queryComponent(IDocument::AShapeComponent::ID     coid,
                                                      const IDocument::AShapeComponent** component) const
{
    if (component == nullptr)
    {
        return Error::INVALID_ARGS;
    }
    *component = nullptr;
    if (coid == EllipseColorComponent::COID)
    {
        *component = static_cast<const EllipseColorComponent*>(this);
        return Error::SUCCESS;
    }
    return Error::COMPONENT_NOT_FOUND;
}

} // namespace Yasha::Tools
