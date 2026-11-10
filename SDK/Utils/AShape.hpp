#ifndef A_SHAPE_HPP
#define A_SHAPE_HPP

#include "Interfaces/IRenderer.hpp"
#include "Utils/ADocumentObject.hpp"
#include "Utils/ColorComponent.hpp"

#include <cassert>
#include <memory>

class AShape : public ADocumentObject
{
public:
    AShape();
    ~AShape() override = 0;

    Color color() const;
    void  setColor(Color color) override final;
}; // class AShape

/* ====================== Implementation ====================== */

inline AShape::AShape()
{
    addComponent(std::make_unique<ColorComponent>());
}

inline AShape::~AShape() = default;

inline Color AShape::color() const
{
    ColorComponent const* component = queryComponent<ColorComponent>();
    assert(component != nullptr);

    return component->color();
}

inline void AShape::setColor(Color const color)
{
    ColorComponent* component = queryComponent<ColorComponent>();
    assert(component != nullptr);

    component->setColor(color);
}

#endif // A_SHAPE_HPP
