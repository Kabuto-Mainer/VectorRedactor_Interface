#ifndef A_SHAPE_HPP
#define A_SHAPE_HPP

#include "Utils/DocumentObject.hpp"
#include "Utils/ShapeGraphic.hpp"
#include "Utils/TransformComponent.hpp"

class AShape : public DocumentObject, public TransformComponent, public ShapeGraphic
{
public:
    virtual ~AShape() = default;

protected:
    explicit AShape(Vec2F const& size) : TransformComponent(size)
    {
    }
};

#endif
