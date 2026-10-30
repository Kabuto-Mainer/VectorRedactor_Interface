#ifndef BOX_F_HPP
#define BOX_F_HPP

#include "SDK/Utils/Vec2F.hpp"

struct BoxF
{
    Vec2F pos;
    Vec2F size;

    BoxF() = default;

    bool contains(const Vec2F& point) const
    {
        return point.x >= pos.x && point.x <= pos.x + size.x && point.y >= pos.y && point.y <= pos.y + size.y;
    }

    BoxF(const Vec2F& pos1, const Vec2F& size1) : pos(pos1), size(size1)
    {
    }
};

#endif
