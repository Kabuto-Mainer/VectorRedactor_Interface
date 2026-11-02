#ifndef VEC_2F_HPP
#define VEC_2F_HPP

#include <cmath>
#include <iostream>

class Vec2F
{
private:
    constexpr static const float PERSISION = 1e-5F;
    float                        x_        = 0.0F;
    float                        y_        = 0.0F;

public:
    Vec2F() = default;
    Vec2F(float x_value, float y_value);
    explicit Vec2F(float value);

    float x() const;
    float y() const;
    void  setX(float x_val);
    void  setY(float y_val);

    Vec2F ortogonal() const;
    float len() const;
    float len2() const;
    float dot(const Vec2F& vec) const;
    Vec2F cwiseProduct(const Vec2F& vec) const;
    void  print() const;

    Vec2F  operator+() const;
    Vec2F  operator+(const Vec2F& vec) const;
    Vec2F& operator+=(const Vec2F& vec);

    Vec2F  operator-() const;
    Vec2F  operator-(const Vec2F& vec) const;
    Vec2F& operator-=(const Vec2F& vec);

    Vec2F        operator*(float scalar) const;
    friend Vec2F operator*(float scalar, const Vec2F& vec);
    Vec2F&       operator*=(float scalar);

    Vec2F  operator/(float scalar) const;
    Vec2F& operator/=(float scalar);

    bool operator==(const Vec2F& vec) const;
    bool operator!=(const Vec2F& vec) const;
};


/* ====================== Implementation ====================== */

Vec2F::Vec2F(float x_value, float y_value) : x_(x_value), y_(y_value)
{
}

Vec2F::Vec2F(float value) : x_(value), y_(value)
{
}

float Vec2F::x() const
{
    return x_;
}

float Vec2F::y() const
{
    return y_;
}

void Vec2F::setX(float x_val)
{
    x_ = x_val;
}

void Vec2F::setY(float y_val)
{
    y_ = y_val;
}

float Vec2F::len2() const
{
    return (x_ * x_) + (y_ * y_);
}

float Vec2F::len() const
{
    return std::sqrtf(len2());
}

float Vec2F::dot(const Vec2F& vec) const
{
    return (x_ * vec.x_) + (y_ * vec.y_);
}

Vec2F Vec2F::ortogonal() const
{
    return {y_, -x_};
}

Vec2F Vec2F::operator+(const Vec2F& vec) const
{
    return {x_ + vec.x_, y_ + vec.y_};
}

Vec2F Vec2F::operator-(const Vec2F& vec) const
{
    return {x_ - vec.x_, y_ - vec.y_};
}

Vec2F Vec2F::operator-() const
{
    return {-x_, -y_};
}

Vec2F Vec2F::operator*(float scalar) const
{
    return {x_ * scalar, y_ * scalar};
}

Vec2F Vec2F::operator/(float scalar) const
{
    return {x_ / scalar, y_ / scalar};
}

Vec2F Vec2F::cwiseProduct(const Vec2F& vec) const
{
    return {x_ * vec.x_, y_ * vec.y_};
}

Vec2F& Vec2F::operator+=(const Vec2F& vec)
{
    x_ += vec.x_;
    y_ += vec.y_;
    return *this;
}

Vec2F& Vec2F::operator-=(const Vec2F& vec)
{
    x_ -= vec.x_;
    y_ -= vec.y_;
    return *this;
}

Vec2F& Vec2F::operator*=(float scalar)
{
    x_ *= scalar;
    y_ *= scalar;
    return *this;
}

Vec2F& Vec2F::operator/=(float scalar)
{
    x_ /= scalar;
    y_ /= scalar;
    return *this;
}

bool Vec2F::operator==(const Vec2F& vec) const
{
    return (*this - vec).len2() < PERSISION * PERSISION;
}

bool Vec2F::operator!=(const Vec2F& vec) const
{
    return !(vec == *this);
}

void Vec2F::print() const
{
    std::cerr << "{ " << x_ << ", " << y_ << " }";
}

#endif
