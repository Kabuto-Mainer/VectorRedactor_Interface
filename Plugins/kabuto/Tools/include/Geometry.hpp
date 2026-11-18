#pragma once

#include "Utils/BoxF.hpp"
#include "Utils/TObject.hpp"
#include "Utils/Vec2F.hpp"

#include <algorithm>

namespace Kabuto
{

namespace Tools
{

/**
 * Builds a normalized box from two arbitrary corners.
 */
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

inline Vec2F globalToDocument(const TObject& document_view, const Vec2F& global_pos)
{
    return document_view.mapGlobalToLocal(global_pos);
}

inline Vec2F documentToGlobal(const TObject& document_view, const Vec2F& document_pos)
{
    return document_view.mapLocalToGlobal(document_pos);
}

inline BoxF documentToGlobal(const TObject& document_view, const BoxF& document_box)
{
    const Vec2F first  = documentToGlobal(document_view, document_box.topLeft());
    const Vec2F second = documentToGlobal(document_view, document_box.topRight());
    const Vec2F third  = documentToGlobal(document_view, document_box.bottomLeft());
    const Vec2F fourth = documentToGlobal(document_view, document_box.bottomRight());

    return BoxF(std::min({first.x(), second.x(), third.x(), fourth.x()}),
                std::min({first.y(), second.y(), third.y(), fourth.y()}),
                std::max({first.x(), second.x(), third.x(), fourth.x()}),
                std::max({first.y(), second.y(), third.y(), fourth.y()}));
}

}; // namespace Tools

}; // namespace Kabuto
