#ifndef TRANSFORM_COMPONENT_HPP
#define TRANSFORM_COMPONENT_HPP

#include "Utils/BoxF.hpp"
#include "Utils/Matrix3F.hpp"
#include "Utils/Vec2F.hpp"

#include <cmath>

class TransformComponent
{
public:
    Matrix3F transform() const
    {
        return transform_;
    }
    void resetTransform()
    {
        transform_ = Matrix3F{};
    }
    void applyTransform(Matrix3F const& transform)
    {
        transform_ = transform * transform_;
    }

    Vec2F size() const
    {
        return size_;
    }

protected:
    explicit TransformComponent(Vec2F const& size) : size_(size)
    {
    }

    Vec2F worldCenter() const
    {
        return transform_.transformPoint(size_ / 2.0F);
    }

    Vec2F worldSize() const
    {
        const float scale_x = std::hypot(transform_[0][0], transform_[1][0]);
        const float scale_y = std::hypot(transform_[0][1], transform_[1][1]);
        return Vec2F{size_.x() * scale_x, size_.y() * scale_y};
    }

    float worldAngle() const
    {
        return std::atan2(transform_[1][0], transform_[0][0]);
    }

    BoxF worldBox() const
    {
        BoxF box{worldSize()};
        box.setCenter(worldCenter());
        return box;
    }

    bool containsPoint(Vec2F const& point) const
    {
        return BoxF{size_}.contains(transform_.inverse().transformPoint(point));
    }

private:
    Matrix3F transform_{};
    Vec2F    size_{};
};

#endif
