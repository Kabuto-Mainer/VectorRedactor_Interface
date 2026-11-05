#ifndef VEC_2F_HPP
#define VEC_2F_HPP

#include <cmath>

class Vec2F
{
public:
    Vec2F();
    ~Vec2F();

    Vec2F(float x_value, float y_value);

    float x() const;
    float y() const;

    void setX(float new_x);
    void setY(float new_y);

    Vec2F operator+() const;
    Vec2F operator-() const;

    Vec2F& operator+=(const Vec2F& vec);
    Vec2F& operator-=(const Vec2F& vec);
    Vec2F& operator*=(float scalar);
    Vec2F& operator/=(float scalar);

    friend Vec2F operator+(Vec2F const& lhs, Vec2F const& rhs);
    friend Vec2F operator-(Vec2F const& lhs, Vec2F const& rhs);
    friend Vec2F operator*(Vec2F const& vec, float scalar);
    friend Vec2F operator*(float scalar, Vec2F const& vec);
    friend Vec2F operator/(Vec2F const& vec, float scalar);

    friend float dotProduct(Vec2F const& lhs, Vec2F const& rhs);

    float len() const;
    float len2() const;

    Vec2F orthogonal() const;

private:
    static constexpr float PRECISION{1e-5F};

private:
    float x_{};
    float y_{};
};


/* ====================== Implementation ====================== */

inline Vec2F::Vec2F()  = default;
inline Vec2F::~Vec2F() = default;

inline Vec2F::Vec2F(float const x_value, float const y_value) : x_{x_value}, y_{y_value}
{
}

inline float Vec2F::x() const
{
    return x_;
}

inline float Vec2F::y() const
{
    return y_;
}

inline void Vec2F::setX(float const new_x)
{
    x_ = new_x;
}

inline void Vec2F::setY(float const new_y)
{
    y_ = new_y;
}

inline Vec2F Vec2F::operator+() const
{
    return Vec2F{+x(), +y()};
}

inline Vec2F Vec2F::operator-() const
{
    return Vec2F{-x(), -y()};
}

inline Vec2F& Vec2F::operator+=(const Vec2F& vec)
{
    setX(x() + vec.x());
    setY(y() + vec.y());
    return *this;
}

inline Vec2F& Vec2F::operator-=(const Vec2F& vec)
{
    setX(x() - vec.x());
    setY(y() - vec.y());
    return *this;
}

inline Vec2F& Vec2F::operator*=(float const scalar)
{
    setX(x() * scalar);
    setY(y() * scalar);
    return *this;
}

inline Vec2F& Vec2F::operator/=(float const scalar)
{
    setX(x() / scalar);
    setY(y() / scalar);
    return *this;
}

inline Vec2F operator+(Vec2F const& lhs, Vec2F const& rhs)
{
    Vec2F result{lhs};
    return result += rhs;
}

inline Vec2F operator-(Vec2F const& lhs, Vec2F const& rhs)
{
    Vec2F result{lhs};
    return result -= rhs;
}

inline Vec2F operator*(Vec2F const& vec, float scalar)
{
    Vec2F result{vec};
    return result *= scalar;
}

inline Vec2F operator*(float scalar, Vec2F const& vec)
{
    Vec2F result{vec};
    return result *= scalar;
}

inline Vec2F operator/(Vec2F const& vec, float scalar)
{
    Vec2F result{vec};
    return result /= scalar;
}

inline float dotProduct(Vec2F const& lhs, Vec2F const& rhs)
{
    return (lhs.x() * rhs.x()) + (lhs.y() * rhs.y());
}

inline float Vec2F::len2() const
{
    return dotProduct(*this, *this);
}

inline float Vec2F::len() const
{
    return std::sqrt(len2());
}

inline Vec2F Vec2F::orthogonal() const
{
    return Vec2F{y(), -x()};
}

#endif
