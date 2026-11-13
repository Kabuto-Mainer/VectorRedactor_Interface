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
    ~EllipseShape() override final;

    EllipseShape(Vec2F const& size);

    bool processEvent(IInputPump::Event const& event) override final;
    void render() override final;

    bool                              hitTest(Vec2F const& pos) const override final;
    virtual BoxF                      axesAlignedBoundingBox() const override final;
    virtual IDocument::TShape::TypeID tid() const override final;

    virtual ApplicationCore::Error queryComponent(IDocument::AShapeComponent::ID coid,
                                                  IDocument::AShapeComponent**   component) override final;
    virtual ApplicationCore::Error queryComponent(IDocument::AShapeComponent::ID     coid,
                                                  IDocument::AShapeComponent const** component) const override final;
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

inline ApplicationCore::Error EllipseShape::queryComponent(IDocument::AShapeComponent::ID coid,
                                                           IDocument::AShapeComponent**   component)
{
    assert(component);

    switch (coid)
    {
        case ColorComponent::COID:
        {
            (*component) = static_cast<ColorComponent*>(this);
            return ApplicationCore::Error::SUCCESS;
        }
        default:
            break;
    }
    (*component) = nullptr;
    return ApplicationCore::Error::INTERFACE_NOT_FOUND;
}

inline ApplicationCore::Error EllipseShape::queryComponent(IDocument::AShapeComponent::ID     coid,
                                                           IDocument::AShapeComponent const** component) const
{
    assert(component);

    switch (coid)
    {
        case ColorComponent::COID:
        {
            (*component) = static_cast<ColorComponent const*>(this);
            return ApplicationCore::Error::SUCCESS;
        }
        default:
            break;
    }
    (*component) = nullptr;
    return ApplicationCore::Error::INTERFACE_NOT_FOUND;
}

}; // namespace VDocument

#endif // ELLIPSE_SHAPE_HPP
