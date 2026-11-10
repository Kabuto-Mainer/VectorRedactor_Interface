#ifndef COLOR_COMPONENT_HPP
#define COLOR_COMPONENT_HPP

#include "Interfaces/IRenderer.hpp"
#include "Utils/ADocumentObjectComponent.hpp"

class ColorComponent final : public ADocumentObjectComponent
{
public:
    constexpr static const CoID COID = 4;

    ColorComponent() = default;
    explicit ColorComponent(Color color);
    ~ColorComponent() override = default;

    Color color() const;
    void  setColor(Color color);

    CoID coid() const override;

private:
    Color color_{};
}; // class ColorComponent

/* ====================== Implementation ====================== */

inline ColorComponent::ColorComponent(Color const color) : color_(color)
{
}

inline Color ColorComponent::color() const
{
    return color_;
}

inline void ColorComponent::setColor(Color const color)
{
    color_ = color;
}

inline CoID ColorComponent::coid() const
{
    return COID;
}

#endif // COLOR_COMPONENT_HPP
