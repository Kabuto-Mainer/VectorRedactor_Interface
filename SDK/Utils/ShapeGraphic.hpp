#ifndef SHAPE_GRAPHIC_HPP
#define SHAPE_GRAPHIC_HPP

#include "Interfaces/IRenderer.hpp"

class ShapeGraphic
{
public:
    Color fillColor() const
    {
        return fill_color_;
    }
    void setFillColor(Color const& color)
    {
        fill_color_ = color;
    }

    Color outlineColor() const
    {
        return outline_color_;
    }
    void setOutlineColor(Color const& color)
    {
        outline_color_ = color;
    }

    float outlineThickness() const
    {
        return outline_thickness_;
    }
    void setOutlineThickness(float const thickness)
    {
        outline_thickness_ = thickness;
    }

protected:
    ShapeGraphic() = default;

private:
    Color fill_color_{0, 0, 0, 0};
    Color outline_color_{0, 0, 0, 0};
    float outline_thickness_{};
};

#endif
