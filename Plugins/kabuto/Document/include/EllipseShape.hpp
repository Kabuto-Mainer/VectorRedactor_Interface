#pragma once

#include "Components.hpp"
#include "Interfaces/IDocument.hpp"

#include <cassert>
#include <cmath>

namespace Kabuto
{

namespace Document
{


class EllipseShape final : public IDocument::TShape, public ColorComponent
{
public:
    static constexpr IDocument::TShape::TypeID ELLIPSE_SHAPE_ID = 1;

    EllipseShape();
    ~EllipseShape() override;

    EllipseShape(Vec2F const& size);

    bool processEvent(IInputPump::Event const& event) override;
    void render() override;

    bool                              hitTest(Vec2F const& pos) const override;
    virtual BoxF                      axesAlignedBoundingBox() const override;
    virtual IDocument::TShape::TypeID tid() const override;

    virtual Error queryComponent(IDocument::AShapeComponent::ID coid, IDocument::AShapeComponent** component) override;
    virtual Error queryComponent(IDocument::AShapeComponent::ID     coid,
                                 IDocument::AShapeComponent const** component) const override;
};

/* ====================== Implementation ====================== */

inline EllipseShape::EllipseShape()  = default;
inline EllipseShape::~EllipseShape() = default;

inline EllipseShape::EllipseShape(Vec2F const& size)
{
    setScaling(size);
}

inline bool EllipseShape::processEvent(IInputPump::Event const& /*event*/)
{
    ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "EllipseShape can't process events\n");
    return false;
}

inline void EllipseShape::render()
{
    IRenderer* renderer{};
    ApplicationCore::instance().queryInterface(IID_RENDERER, reinterpret_cast<void**>(&renderer));
    assert(renderer);

    BoxF to_render{
        Vec2F{std::abs(scaling().x()), std::abs(scaling().y())}
    };
    to_render.setCenter(translation());
    renderer->renderEllipse(color(), to_render, rotation());
}

inline bool EllipseShape::hitTest(Vec2F const& pos) const
{
    /* NOTE: In render(), the size of BoxF is defined by scaling(), so the semi-axes of the ellipse are equal to 0.5 * scaling().
             After mapGlobalToLocal(pos), a point on the boundary of the ellipse has a length of 0.5, and after multiplication by 2.0F, it becomes exactly 1.0F.
    */
    return (mapGlobalToLocal(pos) * 2.0F).len2() <= 1.0F;
}

inline BoxF EllipseShape::axesAlignedBoundingBox() const
{
    /* NOTE: A point on the ellipse boundary in local space is (0.5 * cos(t), 0.5 * sin(t)).
             After applying transform(), its X coordinate is tx + 0.5 * (m00 * cos(t) + m01 * sin(t)),
             whose maximum deviation from tx is 0.5 * sqrt(m00^2 + m01^2) (and analogously for Y).
    */
    Vec2F const extent{0.5F * std::sqrt((transform()[0][0] * transform()[0][0]) + (transform()[0][1] * transform()[0][1])),
                       0.5F * std::sqrt((transform()[1][0] * transform()[1][0]) + (transform()[1][1] * transform()[1][1]))};

    return BoxF{translation().x() - extent.x(), translation().y() - extent.y(), translation().x() + extent.x(),
                translation().y() + extent.y()};
}

inline IDocument::TShape::TypeID EllipseShape::tid() const
{
    return ELLIPSE_SHAPE_ID;
}

inline IDocument::TShape::Error EllipseShape::queryComponent(IDocument::AShapeComponent::ID coid,
                                                             IDocument::AShapeComponent**   component)
{
    if (component == nullptr)
    {
        return Error::INVALID_ARGS;
    }

    switch (coid)
    {
        case ColorComponent::COID:
        {
            (*component) = static_cast<ColorComponent*>(this);
            return Error::SUCCESS;
        }
        default:
            break;
    }
    (*component) = nullptr;
    return Error::COMPONENT_NOT_FOUND;
}

inline IDocument::TShape::Error EllipseShape::queryComponent(IDocument::AShapeComponent::ID     coid,
                                                             IDocument::AShapeComponent const** component) const
{
    if (component == nullptr)
    {
        return Error::INVALID_ARGS;
    }

    switch (coid)
    {
        case ColorComponent::COID:
        {
            (*component) = static_cast<const ColorComponent*>(this);
            return Error::SUCCESS;
        }
        default:
            break;
    }
    (*component) = nullptr;
    return Error::COMPONENT_NOT_FOUND;
}

}; // namespace Document

}; // namespace Kabuto
