#ifndef ELLIPSE_SHAPE_HPP
#define ELLIPSE_SHAPE_HPP

#include "Components.hpp"
#include "Interfaces/IDocument.hpp"

#include <cassert>
#include <cmath>

namespace VDocument
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

inline bool EllipseShape::processEvent(IInputPump::Event const& event)
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
    return mapGlobalToLocal(pos).len2() <= 1.0F;
}

// I can't prove that
inline BoxF EllipseShape::axesAlignedBoundingBox() const
{
    Vec2F const extent{std::sqrt((transform()[0][0] * transform()[0][0]) + (transform()[0][1] * transform()[0][1])),
                       std::sqrt((transform()[1][0] * transform()[1][0]) + (transform()[1][1] * transform()[1][1]))};

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

}; // namespace VDocument

#endif // ELLIPSE_SHAPE_HPP
