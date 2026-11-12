#ifndef COMPONENTS_HPP
#define COMPONENTS_HPP

#include "Interfaces/IRenderer.hpp"
#include "Interfaces/IDocument.hpp"

namespace VDocument
{

/* ====================== TextComponent ====================== */

class TextComponent : public IDocument::AShapeComponent
{
public:
    constexpr static const IDocument::AShapeComponent::ID COID = 6;

    TextComponent() = default;
    ~TextComponent() override = default;

    const IRenderer::TextFormat& getFormat() const;
    void                         setFormat(const IRenderer::TextFormat& format);

    ID coid() const override final;

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

inline IDocument::AShapeComponent::ID TextComponent::coid() const
{
    return TextComponent::COID;
}

/* ====================== ColorComponent ====================== */

class ColorComponent : public IDocument::AShapeComponent
{
public:
    constexpr static const ID COID = 4;

    ColorComponent() = default;
    ~ColorComponent() override = default;

    virtual IRenderer::Color color() const = 0;
    virtual void             setColor(IRenderer::Color color) = 0;

    ID coid() const override final;

protected:
    IRenderer::Color color_{};
}; // class ColorComponent

/* ====================== Implementation ====================== */

inline IDocument::AShapeComponent::ID ColorComponent::coid() const
{
    return ColorComponent::COID;
}

}; // namespace VDocument

#endif // COMPONENTS_HPP
