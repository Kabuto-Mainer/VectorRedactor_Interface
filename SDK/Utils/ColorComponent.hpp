#ifndef COLOR_COMPONENT_HPP
#define COLOR_COMPONENT_HPP

#include "Interfaces/IRenderer.hpp"
#include "Utils/ADocumentObjectComponent.hpp"

class ColorComponent : public ADocumentObjectComponent
{
public:
    constexpr static const CoID COID = 4;

    ColorComponent() = default;
    explicit ColorComponent(IRenderer::Color color);
    ~ColorComponent() override = default;

    IRenderer::Color color() const;
    void             setColor(IRenderer::Color color);

    CoID coid() const override;

protected:
    IRenderer::Color color_{};
}; // class ColorComponent

/* ====================== Implementation ====================== */

inline ColorComponent::ColorComponent(IRenderer::Color const color) : color_(color)
{
}

inline IRenderer::Color ColorComponent::color() const
{
    return color_;
}

inline void ColorComponent::setColor(IRenderer::Color const color)
{
    color_ = color;
}

inline CoID ColorComponent::coid() const
{
    return COID;
}

#endif // COLOR_COMPONENT_HPP
