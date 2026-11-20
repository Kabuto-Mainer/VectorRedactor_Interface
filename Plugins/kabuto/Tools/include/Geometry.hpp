#pragma once

#include "Utils/BoxF.hpp"
#include "Utils/Vec2F.hpp"

#include <algorithm>

namespace Kabuto
{

namespace Tools
{

inline BoxF boxFromCorners(const Vec2F& first, const Vec2F& second)
{
    const Vec2F min{std::min(first.x(), second.x()), std::min(first.y(), second.y())};
    const Vec2F max{std::max(first.x(), second.x()), std::max(first.y(), second.y())};

    return BoxF(min, max);
}

inline bool boxesOverlap(const BoxF& first, const BoxF& second)
{
    return first.minX() <= second.maxX() &&
           second.minX() <= first.maxX() &&
           first.minY() <= second.maxY() &&
           second.minY() <= first.maxY();
}

}; // namespace Tools

}; // namespace Kabuto
