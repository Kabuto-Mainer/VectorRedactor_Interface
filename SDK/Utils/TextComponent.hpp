#ifndef TEXT_COMPONENT_HPP
#define TEXT_COMPONENT_HPP

#include "Interfaces/IRenderer.hpp"
#include "Utils/ADocumentObjectComponent.hpp"

#include <string>
#include <utility>

class TextComponent final : public ADocumentObjectComponent
{
public:
    constexpr static const CoID COID = 6;

    TextComponent() = default;
    TextComponent(std::string text, Color color, IRenderer::FontID font_id, int font_size);
    ~TextComponent() override = default;

    std::string const& text() const;
    void               setText(std::string text);

    Color color() const;
    void  setColor(Color color);

    IRenderer::FontID fontID() const;
    void              setFontID(IRenderer::FontID font_id);

    int  fontSize() const;
    void setFontSize(int font_size);

    CoID coid() const override;

private:
    std::string       text_;
    Color             color_{};
    IRenderer::FontID font_id_{};
    int               font_size_{};
}; // class TextComponent

/* ====================== Implementation ====================== */

inline TextComponent::TextComponent(std::string text, Color const color, IRenderer::FontID const font_id,
                                    int const font_size) :
    text_(std::move(text)), color_(color), font_id_(font_id), font_size_(font_size)
{
}

inline std::string const& TextComponent::text() const
{
    return text_;
}

inline void TextComponent::setText(std::string text)
{
    text_ = std::move(text);
}

inline Color TextComponent::color() const
{
    return color_;
}

inline void TextComponent::setColor(Color const color)
{
    color_ = color;
}

inline IRenderer::FontID TextComponent::fontID() const
{
    return font_id_;
}

inline void TextComponent::setFontID(IRenderer::FontID const font_id)
{
    font_id_ = font_id;
}

inline int TextComponent::fontSize() const
{
    return font_size_;
}

inline void TextComponent::setFontSize(int const font_size)
{
    font_size_ = font_size;
}

inline CoID TextComponent::coid() const
{
    return COID;
}

#endif // TEXT_COMPONENT_HPP
