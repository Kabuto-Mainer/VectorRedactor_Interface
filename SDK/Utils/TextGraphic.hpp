#ifndef TEXT_GRAPHIC_HPP
#define TEXT_GRAPHIC_HPP

#include "Interfaces/IRenderer.hpp"

#include <string>

class TextGraphic
{
public:
    std::string text() const
    {
        return text_;
    }
    void setText(std::string const& text)
    {
        text_ = text;
    }

    Color color() const
    {
        return color_;
    }
    void setColor(Color const& color)
    {
        color_ = color;
    }

    IRenderer::FontID fontID() const
    {
        return font_id_;
    }
    void setFontID(IRenderer::FontID const font_id)
    {
        font_id_ = font_id;
    }

    int fontSize() const
    {
        return font_size_;
    }
    void setFontSize(int const font_size)
    {
        font_size_ = font_size;
    }

protected:
    TextGraphic() = default;

private:
    std::string       text_;
    Color             color_{0, 0, 0, 0};
    IRenderer::FontID font_id_{};
    int               font_size_{};
};

#endif
