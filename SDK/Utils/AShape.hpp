#ifndef A_SHAPE_HPP
#define A_SHAPE_HPP

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Utils/ADocumentObject.hpp"
#include "Utils/ADocumentObjectComponent.hpp"
#include "Utils/ColorComponent.hpp"

#include <cassert>
#include <memory>

class AShape : public ADocumentObject, public ColorComponent
{
public:
    AShape();
    ~AShape() override = 0;
    ApplicationCore::Error queryComponent(CoID coid, ADocumentObjectComponent const** component) const override;
    ApplicationCore::Error queryComponent(CoID coid, ADocumentObjectComponent** component) override;

}; // class AShape

/* ====================== Implementation ====================== */

inline AShape::AShape()  = default;
inline AShape::~AShape() = default;


ApplicationCore::Error AShape::queryComponent(CoID coid, ADocumentObjectComponent const** component) const
{
    assert(component);

    switch (coid)
    {
        case ColorComponent::COID:
        {
            (*component) = static_cast<const ColorComponent*>(this);
            return ApplicationCore::Error::SUCCESS;
        }
        default:
            break;
    }
    (*component) = nullptr;
    return ApplicationCore::Error::INTERFACE_NOT_FOUND;
}

ApplicationCore::Error AShape::queryComponent(CoID coid, ADocumentObjectComponent** component)
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

#endif // A_SHAPE_HPP
