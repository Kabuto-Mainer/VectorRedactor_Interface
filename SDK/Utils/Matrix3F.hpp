#ifndef MATRIX_3F_HPP
#define MATRIX_3F_HPP

#include "Utils/Vec2F.hpp"

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>

// Матрица для двумерных аффинных преобразований в однородных координатах.
// Векторы считаются столбцами: p' = M * p.
class Matrix3F
{
public:
    static constexpr std::size_t DIMENSION{3};
    using Storage = std::array<std::array<float, DIMENSION>, DIMENSION>;

public:
    static Matrix3F identity();
    static Matrix3F translation(float translation_x, float translation_y);
    static Matrix3F scaling(float scale_x, float scale_y);
    static Matrix3F rotation(float radians);
    static Matrix3F shear(float x_by_y, float y_by_x);

public:
    Matrix3F();
    ~Matrix3F();

    explicit Matrix3F(const Storage& values);

    std::array<float, DIMENSION>&       operator[](std::size_t row);
    const std::array<float, DIMENSION>& operator[](std::size_t row) const;

    Matrix3F operator+() const;
    Matrix3F operator-() const;

    Matrix3F& operator+=(const Matrix3F& rhs);
    Matrix3F& operator-=(const Matrix3F& rhs);
    Matrix3F& operator*=(float scalar);
    Matrix3F& operator*=(const Matrix3F& rhs);

    friend Matrix3F operator+(const Matrix3F& lhs, const Matrix3F& rhs);
    friend Matrix3F operator-(const Matrix3F& lhs, const Matrix3F& rhs);
    friend Matrix3F operator*(float scalar, const Matrix3F& matrix);
    friend Matrix3F operator*(const Matrix3F& matrix, float scalar);
    friend Matrix3F operator*(const Matrix3F& lhs, const Matrix3F& rhs);

    Vec2F transformPoint(const Vec2F& point) const;

    Matrix3F transposed() const;

    float    determinant() const;
    Matrix3F inverted() const;

private:
    static constexpr float DEFAULT_EPSILON{1e-7F};

private:
    Storage values_{};
}; // class Matrix3F

/* ======================== Implementation ======================== */

inline Matrix3F Matrix3F::identity()
{
    return Matrix3F{};
}

inline Matrix3F Matrix3F::translation(float const translation_x, float const translation_y)
{
    return Matrix3F{Storage{{{1.0F, 0.0F, translation_x}, {0.0F, 1.0F, translation_y}, {0.0F, 0.0F, 1.0F}}}};
}

inline Matrix3F Matrix3F::scaling(float const scale_x, float const scale_y)
{
    return Matrix3F{Storage{{{scale_x, 0.0F, 0.0F}, {0.0F, scale_y, 0.0F}, {0.0F, 0.0F, 1.0F}}}};
}

inline Matrix3F Matrix3F::rotation(float const radians)
{
    const float cose = std::cos(radians);
    const float sine = std::sin(radians);

    return Matrix3F{Storage{{{cose, -sine, 0.0F}, {sine, cose, 0.0F}, {0.0F, 0.0F, 1.0F}}}};
}

inline Matrix3F Matrix3F::shear(float const x_by_y, float const y_by_x)
{
    return Matrix3F{Storage{{{1.0F, x_by_y, 0.0F}, {y_by_x, 1.0F, 0.0F}, {0.0F, 0.0F, 1.0F}}}};
}

inline Matrix3F::Matrix3F() : values_{Storage{{{1.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F}, {0.0F, 0.0F, 1.0F}}}}
{
}

inline Matrix3F::~Matrix3F() = default;

inline Matrix3F::Matrix3F(const Storage& values) : values_{values}
{
}

inline std::array<float, Matrix3F::DIMENSION>& Matrix3F::operator[](std::size_t const row)
{
    return values_[row];
}

inline const std::array<float, Matrix3F::DIMENSION>& Matrix3F::operator[](std::size_t const row) const
{
    return values_[row];
}

inline Matrix3F Matrix3F::operator+() const
{
    Matrix3F result{Storage{}};
    for (std::size_t row = 0; row < DIMENSION; ++row)
    {
        for (std::size_t column = 0; column < DIMENSION; ++column)
        {
            result[row][column] = +(*this)[row][column];
        }
    }
    return *this;
}

inline Matrix3F Matrix3F::operator-() const
{
    Matrix3F result{Storage{}};
    for (std::size_t row = 0; row < DIMENSION; ++row)
    {
        for (std::size_t column = 0; column < DIMENSION; ++column)
        {
            result[row][column] = -(*this)[row][column];
        }
    }
    return *this;
}

inline Matrix3F& Matrix3F::operator+=(const Matrix3F& rhs)
{
    for (std::size_t row = 0; row < DIMENSION; ++row)
    {
        for (std::size_t column = 0; column < DIMENSION; ++column)
        {
            (*this)[row][column] += rhs[row][column];
        }
    }
    return *this;
}

inline Matrix3F& Matrix3F::operator-=(const Matrix3F& rhs)
{
    for (std::size_t row = 0; row < DIMENSION; ++row)
    {
        for (std::size_t column = 0; column < DIMENSION; ++column)
        {
            (*this)[row][column] -= rhs[row][column];
        }
    }
    return *this;
}

inline Matrix3F& Matrix3F::operator*=(float const scalar)
{
    for (std::size_t row = 0; row < DIMENSION; ++row)
    {
        for (std::size_t column = 0; column < DIMENSION; ++column)
        {
            (*this)[row][column] *= scalar;
        }
    }
    return *this;
}

inline Matrix3F& Matrix3F::operator*=(const Matrix3F& rhs)
{
    return *this = *this * rhs;
}

inline Matrix3F operator+(const Matrix3F& lhs, const Matrix3F& rhs)
{
    Matrix3F result{lhs};
    return result += rhs;
}

inline Matrix3F operator-(const Matrix3F& lhs, const Matrix3F& rhs)
{
    Matrix3F result{lhs};
    return result -= rhs;
}

inline Matrix3F operator*(float const scalar, const Matrix3F& matrix)
{
    Matrix3F result{matrix};
    return result *= scalar;
}

inline Matrix3F operator*(const Matrix3F& matrix, float const scalar)
{
    Matrix3F result{matrix};
    return result *= scalar;
}

inline Matrix3F operator*(const Matrix3F& lhs, const Matrix3F& rhs)
{
    Matrix3F result{Matrix3F::Storage{}};
    for (std::size_t row = 0; row < Matrix3F::DIMENSION; ++row)
    {
        for (std::size_t k = 0; k < Matrix3F::DIMENSION; ++k)
        {
            for (std::size_t column = 0; column < Matrix3F::DIMENSION; ++column)
            {
                result[row][column] += lhs[row][k] * rhs[k][column];
            }
        }
    }
    return result;
}

inline Vec2F Matrix3F::transformPoint(const Vec2F& point) const
{
    const float transformed_x = ((*this)[0][0] * point.x()) + ((*this)[0][1] * point.y()) + (*this)[0][2];
    const float transformed_y = ((*this)[1][0] * point.x()) + ((*this)[1][1] * point.y()) + (*this)[1][2];
    const float homogeneous_w = ((*this)[2][0] * point.x()) + ((*this)[2][1] * point.y()) + (*this)[2][2];
    assert(std::abs(homogeneous_w - 1.0F) <= 0);

    return {transformed_x / homogeneous_w, transformed_y / homogeneous_w};
}

inline Matrix3F Matrix3F::transposed() const
{
    Matrix3F result{Storage{}};
    for (std::size_t row = 0; row < DIMENSION; ++row)
    {
        for (std::size_t column = 0; column < DIMENSION; ++column)
        {
            result.values_[row][column] = (*this)[column][row];
        }
    }
    return result;
}

inline float Matrix3F::determinant() const
{
    return ((*this)[0][0] * (((*this)[1][1] * (*this)[2][2]) - ((*this)[1][2] * (*this)[2][1]))) -
           ((*this)[0][1] * (((*this)[1][0] * (*this)[2][2]) - ((*this)[1][2] * (*this)[2][0]))) +
           ((*this)[0][2] * (((*this)[1][0] * (*this)[2][1]) - ((*this)[1][1] * (*this)[2][0])));
}

inline Matrix3F Matrix3F::inverted() const
{
    const float det = determinant();
    if (std::abs(det) <= DEFAULT_EPSILON)
    {
        return identity();
    }

    const float inv = 1.0F / det;
    Storage     inverse_values{};

    inverse_values[0][0] = (((*this)[1][1] * (*this)[2][2]) - ((*this)[1][2] * (*this)[2][1])) * inv;
    inverse_values[0][1] = (((*this)[0][2] * (*this)[2][1]) - ((*this)[0][1] * (*this)[2][2])) * inv;
    inverse_values[0][2] = (((*this)[0][1] * (*this)[1][2]) - ((*this)[0][2] * (*this)[1][1])) * inv;
    inverse_values[1][0] = (((*this)[1][2] * (*this)[2][0]) - ((*this)[1][0] * (*this)[2][2])) * inv;
    inverse_values[1][1] = (((*this)[0][0] * (*this)[2][2]) - ((*this)[0][2] * (*this)[2][0])) * inv;
    inverse_values[1][2] = (((*this)[0][2] * (*this)[1][0]) - ((*this)[0][0] * (*this)[1][2])) * inv;
    inverse_values[2][0] = (((*this)[1][0] * (*this)[2][1]) - ((*this)[1][1] * (*this)[2][0])) * inv;
    inverse_values[2][1] = (((*this)[0][1] * (*this)[2][0]) - ((*this)[0][0] * (*this)[2][1])) * inv;
    inverse_values[2][2] = (((*this)[0][0] * (*this)[1][1]) - ((*this)[0][1] * (*this)[1][0])) * inv;

    return Matrix3F{inverse_values};
}

#endif
