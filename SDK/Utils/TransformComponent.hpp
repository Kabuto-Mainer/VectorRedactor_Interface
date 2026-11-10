#ifndef TRANSFORM_COMPONENT_HPP
#define TRANSFORM_COMPONENT_HPP

#include "Utils/ADocumentObjectComponent.hpp"
#include "Utils/Transformable.hpp"

class TransformComponent final : public ADocumentObjectComponent, public Transformable
{
public:
    constexpr static const CoID COID = 5;

    TransformComponent()           = default;
    ~TransformComponent() override = default;

    CoID coid() const override;
}; // class TransformComponent

/* ====================== Implementation ====================== */

inline CoID TransformComponent::coid() const
{
    return COID;
}

#endif // TRANSFORM_COMPONENT_HPP
