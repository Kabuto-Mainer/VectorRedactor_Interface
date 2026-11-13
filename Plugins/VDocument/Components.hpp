#ifndef COMPONENTS_HPP
#define COMPONENTS_HPP

#include "Interfaces/IDocument.hpp"
#include "Interfaces/IRenderer.hpp"

namespace VDocument
{

/* ====================== TextComponent ====================== */

class TextComponent : public IDocument::AShapeComponent
{
public:
    constexpr static const IDocument::AShapeComponent::ID COID = 6;

    TextComponent()           = default;
    ~TextComponent() override = default;

    const IRenderer::TextFormat& getFormat() const;
    void                         setFormat(const IRenderer::TextFormat& format);

protected:
    IRenderer::TextFormat format_{};
}; // class TextComponent

/* ====================== Implementation ====================== */

inline const IRenderer::TextFormat& TextComponent::getFormat() const
{
    return format_;
}

inline void TextComponent::setFormat(const IRenderer::TextFormat& format)
{
    format_ = format;
}

/* ====================== ColorComponent ====================== */

class ColorComponent : public IDocument::AShapeComponent
{
public:
    constexpr static const ID COID = 4;

    ColorComponent()           = default;
    ~ColorComponent() override = default;

    IRenderer::Color color() const;
    void             setColor(const IRenderer::Color& color);

protected:
    IRenderer::Color color_;
}; // class ColorComponent

/* ====================== Implementation ====================== */

inline IRenderer::Color ColorComponent::color() const
{
    return color_;
}

inline void ColorComponent::setColor(const IRenderer::Color& color)
{
    color_ = color;
}

}; // namespace VDocument

#endif // COMPONENTS_HPP
