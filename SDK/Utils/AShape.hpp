#ifndef A_SHAPE_HPP
#define A_SHAPE_HPP

#include "SDK/Interfaces/IRenderer.hpp"
#include "SDK/Utils/AObject.hpp"
#include "SDK/Utils/Matrix3F.hpp"

class AShape : public AObject
{
public:
    virtual ~AShape() = default;

    virtual bool hitTest(Vec2F const& point) const = 0;



    Matrix3F transform() const
    {
        return transform_;
    }
    void resetTransform()
    {
        transform_ = Matrix3F{};
    }
    virtual void applyTransform(Matrix3F const& transform)
    {
        transform_ = transform * transform_;
    }



    Color_t fillColor() const
    {
        return fill_color_;
    }
    void setFillColor(Color_t const& color)
    {
        fill_color_ = color;
    }

    Color_t outlineColor() const
    {
        return outline_color_;
    }
    void setOutlineColor(Color_t const& color)
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
    explicit AShape() = default;

private:
    Matrix3F transform_{};

    Color_t fill_color_{0, 0, 0, 0};
    Color_t outline_color_{0, 0, 0, 0};
    float   outline_thickness_{};
};

#endif
