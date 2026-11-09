#ifndef TRANSFORMABLE_HPP
#define TRANSFORMABLE_HPP

#include "Utils/Math/Matrix3F.hpp"
#include "Utils/Math/Vec2F.hpp"

#include <cassert>
#include <cmath>

// Transformable object designates transformation of
// [-1; 1] x [-1; 1] square.
//
// This transformation is defined by 3 parameters
// (even though such a decription is redundant):
// 1. Scaling  with center in (0; 0)
// 2. Rotation with center in (0; 0)
// 3. Translation of          (0; 0)
//
// Transformable allows one to manage this transformation by
// variation of this parameters or by applying 3x3 matrix
// directly as they chooce
//
// This may be useful, because one could bind their own object
// to this [-1; 1] x [-1; 1] square, redirect every transformation of
// original object to Transformable, and query position of specified
// point by applying Transformable matrix to this point, when needed

class Transformable
{
public:
    Transformable();
    ~Transformable();

    // It is unspecified which one of equivalent parameter sets
    // will be provided, but it's guaranteed, that consecutive
    // get-queries will receive values defining correct transformation
    //
    // Note: equivalent representations may differ from what was set.
    // E.g. setScaling({-1, -1}) may be reported back as
    // scaling() == {1, 1}, rotation() == pi, since
    // S(-1,-1) == R(pi). The composed transform() is always preserved

    Vec2F const& scaling() const;
    float        rotation() const;
    Vec2F const& translation() const;

    Matrix3F const& transform() const;

    void setScaling(Vec2F const& new_scaling);
    void setRotation(float new_rotation);
    void setTranslation(Vec2F const& new_translation);

    void setTransform(Matrix3F const& new_transform);

    void scale(Vec2F const& factor);
    void rotate(float angle);
    void translate(Vec2F const& offset);

    void applyTransform(Matrix3F const& additional_transform);

private:
    void updateParameters() const;
    void updateMatrix() const;

private:
    mutable Vec2F scaling_{1.0F, 1.0F};
    mutable float rotation_{};
    mutable Vec2F translation_{};

    mutable Matrix3F transform_{Matrix3F::identity()};

    mutable bool parameters_need_update_{};
    mutable bool matrix_need_update_{};
};

/* ====================== Implementation ====================== */

inline Transformable::Transformable()  = default;
inline Transformable::~Transformable() = default;

inline Vec2F const& Transformable::scaling() const
{
    updateParameters();
    return scaling_;
}

inline float Transformable::rotation() const
{
    updateParameters();
    return rotation_;
}

inline Vec2F const& Transformable::translation() const
{
    updateParameters();
    return translation_;
}

inline Matrix3F const& Transformable::transform() const
{
    updateMatrix();
    return transform_;
}

inline void Transformable::setScaling(Vec2F const& new_scaling)
{
    updateParameters();
    matrix_need_update_ = true;

    scaling_ = new_scaling;
}

inline void Transformable::setRotation(float const new_rotation)
{
    updateParameters();
    matrix_need_update_ = true;

    rotation_ = new_rotation;
}

inline void Transformable::setTranslation(Vec2F const& new_translation)
{
    updateParameters();
    matrix_need_update_ = true;

    translation_ = new_translation;
}

inline void Transformable::setTransform(Matrix3F const& new_transform)
{
    matrix_need_update_     = false;
    parameters_need_update_ = true;

    transform_ = new_transform;
}

inline void Transformable::scale(Vec2F const& factor)
{
    setScaling(componentWiseProduct(scaling(), factor));
}

inline void Transformable::rotate(float const angle)
{
    setRotation(rotation() + angle);
}

inline void Transformable::translate(Vec2F const& offset)
{
    setTranslation(translation() + offset);
}

inline void Transformable::applyTransform(Matrix3F const& additional_transform)
{
    setTransform(additional_transform * transform());
}

inline void Transformable::updateParameters() const
{
    if (!parameters_need_update_)
    {
        return;
    }
    assert(!matrix_need_update_);

    assert(std::abs(transform_[2][0]) <= 0 && std::abs(transform_[2][1]) <= 0 && std::abs(transform_[2][2] - 1) <= 0);
    translation_ = Vec2F{transform_[0][2], transform_[1][2]};

    static constexpr float EPSILON{1e-7F};
    static_assert(EPSILON >= 0.0F);
    assert(std::abs((transform_[0][0] * transform_[0][1]) + (transform_[1][0] * transform_[1][1])) <= EPSILON);

    scaling_ = Vec2F{std::sqrt((transform_[0][0] * transform_[0][0]) + (transform_[1][0] * transform_[1][0])),
                     std::sqrt((transform_[0][1] * transform_[0][1]) + (transform_[1][1] * transform_[1][1]))};

    if (std::abs(scaling_.x()) <= EPSILON)
    {
        rotation_ = std::atan2(-transform_[0][1], transform_[1][1]);
    }
    else
    {
        rotation_ = std::atan2(transform_[1][0], transform_[0][0]);

        float const det2x2{(transform_[0][0] * transform_[1][1]) - (transform_[0][1] * transform_[1][0])};
        if (det2x2 < -EPSILON)
        {
            scaling_.setY(-scaling_.y());
        }
    }

    parameters_need_update_ = false;
}

inline void Transformable::updateMatrix() const
{
    if (!matrix_need_update_)
    {
        return;
    }
    assert(!parameters_need_update_);

    transform_ = Matrix3F::translation(translation_) * Matrix3F::rotation(rotation_) * Matrix3F::scaling(scaling_);

    matrix_need_update_ = false;
}

#endif
