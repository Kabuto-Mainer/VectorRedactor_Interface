#ifndef BOX_F_HPP
#define BOX_F_HPP

#include "Utils/Vec2F.hpp"

class BoxF
{
public:
    BoxF() = default;
    BoxF(float left_top_x, float left_top_y, float right_bottom_x, float right_bottom_y);
    BoxF(const Vec2F& left_top, const Vec2F& right_bottom);
    explicit BoxF(const Vec2F& size);
    ~BoxF() = default;

    bool  contains(const Vec2F& point) const;
    Vec2F size() const;
    Vec2F center() const;

    Vec2F topLeft() const;
    Vec2F topRight() const;
    Vec2F bottomLeft() const;
    Vec2F bottomRight() const;

    float left() const;
    float right() const;
    float top() const;
    float bottom() const;

    void setLeft(float x);
    void setRight(float x);
    void setTop(float y);
    void setBottom(float y);

    void setTopLeft(const Vec2F& point);
    void setTopRight(const Vec2F& point);
    void setBottomLeft(const Vec2F& point);
    void setBottomRight(const Vec2F& point);
    void setCenter(const Vec2F& point);

private:
    Vec2F left_top_, right_bottom_;
};

/* ======================= Implementation ======================= */

BoxF::BoxF(float left_top_x, float left_top_y, float right_bottom_x, float right_bottom_y) :
    left_top_(Vec2F{left_top_x, left_top_y}), right_bottom_(Vec2F{right_bottom_x, right_bottom_y})
{
}

BoxF::BoxF(const Vec2F& left_top, const Vec2F& right_bottom) : left_top_(left_top), right_bottom_(right_bottom)
{
}

BoxF::BoxF(const Vec2F& size) : left_top_(Vec2F{0.0F, 0.0F}), right_bottom_(size)
{
}

bool BoxF::contains(const Vec2F& point) const
{
    return (left() <= point.x() && point.x() <= right() && top() <= point.y() && point.y() <= bottom());
}
/* ------------------- getters ------------------- */

Vec2F BoxF::size() const
{
    return right_bottom_ - left_top_;
}

Vec2F BoxF::center() const
{
    return (left_top_ + right_bottom_) / 2;
}

Vec2F BoxF::topLeft() const
{
    return Vec2F{left(), top()};
}

Vec2F BoxF::topRight() const
{
    return Vec2F{right(), top()};
}

Vec2F BoxF::bottomLeft() const
{
    return Vec2F{left(), bottom()};
}

Vec2F BoxF::bottomRight() const
{
    return Vec2F{right(), bottom()};
}

float BoxF::left() const
{
    return left_top_.x();
}

float BoxF::right() const
{
    return right_bottom_.x();
}

float BoxF::top() const
{
    return left_top_.y();
}

float BoxF::bottom() const
{
    return right_bottom_.y();
}
/* ------------------- setters ------------------- */

void BoxF::setLeft(float x)
{
    left_top_.setX(x);
}

void BoxF::setRight(float x)
{
    right_bottom_.setX(x);
}

void BoxF::setTop(float y)
{
    left_top_.setY(y);
}

void BoxF::setBottom(float y)
{
    right_bottom_.setY(y);
}

void BoxF::setTopLeft(const Vec2F& point)
{
    setLeft(point.x());
    setTop(point.y());
}

void BoxF::setTopRight(const Vec2F& point)
{
    setRight(point.x());
    setTop(point.y());
}

void BoxF::setBottomLeft(const Vec2F& point)
{
    setLeft(point.x());
    setBottom(point.y());
}

void BoxF::setBottomRight(const Vec2F& point)
{
    setRight(point.x());
    setBottom(point.y());
}

void BoxF::setCenter(const Vec2F& point)
{
    Vec2F size    = this->size();
    left_top_     = point - size / 2;
    right_bottom_ = point + size / 2;
}

#endif
