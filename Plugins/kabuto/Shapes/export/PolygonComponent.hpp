#pragma once

#include "Interfaces/IDocument.hpp"
#include "Utils/Vec2F.hpp"
#include "libs/libtess2/Include/tesselator.h"

#include <cstddef>
#include <list>
#include <vector>

namespace Kabuto
{

namespace Shapes
{

struct Vertex
{
    Vec2F x0;
    Vec2F x1;
    Vec2F x2;

    bool hitTest(const Vec2F& point) const;
}; // struct Vertex

class PolygonComponent : public IDocument::AShapeComponent
{
public:
    constexpr static const IDocument::AShapeComponent::ID COID = 124'235'123;

    explicit PolygonComponent(std::list<Vec2F> points = {});
    ~PolygonComponent() override;

    PolygonComponent(const PolygonComponent&)            = delete;
    PolygonComponent(PolygonComponent&&)                 = delete;
    PolygonComponent& operator=(const PolygonComponent&) = delete;
    PolygonComponent& operator=(PolygonComponent&&)      = delete;

    /** USE ONLY LOCAL COORDINATES */

    const std::list<Vec2F>&    points() const;
    bool                       setPoints(const std::list<Vec2F>& points);
    const std::vector<Vertex>& vertex() const;

    virtual bool movePoint(std::size_t logic_idx, const Vec2F& new_pos) = 0;

protected:
    void update() const;

    std::list<Vec2F> points_;

    struct
    {
        mutable std::vector<Vertex> vertex;
        mutable std::vector<float>  coords;
        mutable TESStesselator*     tesselator = nullptr;
    } cache_;

    mutable bool cache_dirty_ = true;
}; // class PolygonComponent

} // namespace Shapes

} // namespace Kabuto
