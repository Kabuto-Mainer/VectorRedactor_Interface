#ifndef RECTANGLE_SHAPE_HPP
#define RECTANGLE_SHAPE_HPP

#include "Components.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace VDocument
{

class RectangleShape final : public IDocument::TShape, public ColorComponent
{
public:
    static constexpr IDocument::TShape::TypeID RECTANGLE_SHAPE_ID = 0;

    RectangleShape();
    ~RectangleShape();

    RectangleShape(Vec2F const& size);

    bool processEvent(IInputPump::Event const& event) override;
    void render() override;

    bool                              hitTest(Vec2F const& pos) const override;
    virtual BoxF                      axesAlignedBoundingBox() const override;
    virtual IDocument::TShape::TypeID tid() const override;

    virtual ApplicationCore::Error queryComponent(IDocument::AShapeComponent::ID coid,
                                                  IDocument::AShapeComponent**   component) override;
    virtual ApplicationCore::Error queryComponent(IDocument::AShapeComponent::ID     coid,
                                                  IDocument::AShapeComponent const** component) const override;
};

/* ====================== Implementation ====================== */

inline RectangleShape::RectangleShape()  = default;
inline RectangleShape::~RectangleShape() = default;

inline RectangleShape::RectangleShape(Vec2F const& size)
{
    setScaling(size);
}

inline bool RectangleShape::processEvent(IInputPump::Event const& event)
{
    ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "RectangleShape can't process events\n");
    return false;
}

inline void RectangleShape::render()
{
    IRenderer* renderer{};
    ApplicationCore::instance().queryInterface(IID_RENDERER, reinterpret_cast<void**>(&renderer));
    assert(renderer);

    BoxF to_render{
        Vec2F{std::abs(scaling().x()), std::abs(scaling().y())}
    };
    to_render.setCenter(translation());
    renderer->renderRect(color(), to_render, rotation());

    assert(children().empty());
}

inline bool RectangleShape::hitTest(Vec2F const& pos) const
{
    return BoxF{-1, -1, 1, 1}.contains(mapGlobalToLocal(pos));
}

inline BoxF RectangleShape::axesAlignedBoundingBox() const
{
    Vec2F const vertex1{transform().transformPoint(Vec2F{-1, -1})};
    Vec2F const vertex2{transform().transformPoint(Vec2F{-1, +1})};
    Vec2F const vertex3{transform().transformPoint(Vec2F{+1, -1})};
    Vec2F const vertex4{transform().transformPoint(Vec2F{+1, +1})};

    return BoxF{std::min({vertex1.x(), vertex2.x(), vertex3.x(), vertex4.x()}),
                std::min({vertex1.y(), vertex2.y(), vertex3.y(), vertex4.y()}),
                std::max({vertex1.x(), vertex2.x(), vertex3.x(), vertex4.x()}),
                std::max({vertex1.y(), vertex2.y(), vertex3.y(), vertex4.y()})};
}

inline IDocument::TShape::TypeID RectangleShape::tid() const
{
    return RECTANGLE_SHAPE_ID;
}

inline ApplicationCore::Error RectangleShape::queryComponent(IDocument::AShapeComponent::ID coid,
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

inline ApplicationCore::Error RectangleShape::queryComponent(IDocument::AShapeComponent::ID     coid,
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

#endif
