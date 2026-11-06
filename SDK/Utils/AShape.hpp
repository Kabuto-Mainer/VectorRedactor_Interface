#ifndef A_SHAPE_HPP
#define A_SHAPE_HPP

#include "Utils/AObject.hpp"
#include "Utils/Transformable.hpp"

// forward declaration
class BoxF;
class Color;

class AShape : public AObject, public Transformable
{
public:
    AShape();
    ~AShape() override = 0;

    Vec2F mapLocalToGlobal(Vec2F const& pos) const override final;
    Vec2F mapGlobalToLocal(Vec2F const& pos) const override final;

    virtual Color&       color()       = 0;
    virtual Color const& color() const = 0;

    virtual BoxF axesAlignedBoundingBox() const = 0;
};

/* ====================== Implementation ====================== */

inline AShape::AShape()  = default;
inline AShape::~AShape() = default;

inline Vec2F AShape::mapLocalToGlobal(Vec2F const& pos) const
{
    return parent()->mapLocalToGlobal(transform().transformPoint(pos));
}

inline Vec2F AShape::mapGlobalToLocal(Vec2F const& pos) const
{
    return transform().inverted().transformPoint(parent()->mapGlobalToLocal(pos));
}

#endif
