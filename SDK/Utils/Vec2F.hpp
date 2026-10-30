#ifndef VEC_2F_HPP
#define VEC_2F_HPP

struct Vec2F
{
    float x = 0.0;
    float y = 0.0;

    Vec2F() = default;
    Vec2F(float x_value, float y_value) : x(x_value), y(y_value)
    {
    }

    explicit Vec2F(float value) : x(value), y(value)
    {
    }

    float length() const;
    float dot(const Vec2F& vec) const
    {
        return (x * vec.x) + (y * vec.y);
    }

    Vec2F ortogonal() const
    {
        return {y, -x};
    }

    Vec2F operator+(const Vec2F& vec) const
    {
        return {x + vec.x, y + vec.y};
    }
    Vec2F operator-(const Vec2F& vec) const
    {
        return {x - vec.x, y - vec.y};
    }
    Vec2F operator-() const
    {
        return {-x, -y};
    }
    Vec2F operator*(float scalar) const
    {
        return {x * scalar, y * scalar};
    }
    Vec2F operator*(const Vec2F& vec) const
    {
        return cwiseProduct(vec);
    }
    Vec2F operator/(float scalar) const
    {
        return {x / scalar, y / scalar};
    }
    Vec2F cwiseProduct(const Vec2F& vec) const
    {
        return {x * vec.x, y * vec.y};
    }

    Vec2F& operator+=(const Vec2F& vec)
    {
        x += vec.x;
        y += vec.y;
        return *this;
    }
    Vec2F& operator-=(const Vec2F& vec)
    {
        x -= vec.x;
        y -= vec.y;
        return *this;
    }
    Vec2F& operator*=(float scalar)
    {
        x *= scalar;
        y *= scalar;
        return *this;
    }
    Vec2F& operator/=(float scalar)
    {
        x /= scalar;
        y /= scalar;
        return *this;
    }

    bool operator==(const Vec2F& vec) const
    {
        return x == vec.x && y == vec.y;
    }
    double operator^(const Vec2F& vec) const
    {
        return dot(vec);
    }

    void print() const;
    bool operator!=(const Vec2F& vec) const
    {
        return !(*this == vec);
    }
};

Vec2F operator*(float scalar, const Vec2F& vector);

#endif
