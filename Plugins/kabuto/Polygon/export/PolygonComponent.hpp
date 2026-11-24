#pragma once

#include "Interfaces/IDocument.hpp"
#include "Utils/Vec2F.hpp"
#include "libs/libtess2/Include/tesselator.h"

#include <cassert>
#include <cstddef>
#include <vector>

namespace Kabuto
{

namespace PolygonHuiny
{

struct Vertex
{
    Vec2F x0;
    Vec2F x1;
    Vec2F x2;
}; // struct Vertex

class PolygonComponent : public IDocument::AShapeComponent
{
public:
    constexpr static const IDocument::AShapeComponent::ID COID = 124'235'123;

    PolygonComponent(std::list<Vec2F> points);
    ~PolygonComponent() override;

    const std::list<Vec2F>&    points() const;
    const std::vector<Vertex>& vertex();
    virtual bool               movePoint(size_t logic_idx, const Vec2F& new_pos) = 0;

protected:
    void update();

    std::list<Vec2F> points_;

    struct
    {
        std::vector<Vertex> vertex;
        std::vector<float>  coords;
        TESStesselator*     tesselator = nullptr;
    } cache_;

    bool cache_dirty_ = true;
}; // class PolygonComponent


inline PolygonComponent::PolygonComponent(std::list<Vec2F> points) : points_(points)
{
    cache_.tesselator = tessNewTess(nullptr);
    if (cache_.tesselator == nullptr)
    {
        assert(0);
    }
    update();
}

inline PolygonComponent::~PolygonComponent()
{
    if (cache_.tesselator)
    {
        tessDeleteTess(cache_.tesselator);
    }
}

inline void PolygonComponent::update()
{
    cache_.vertex.clear();
    cache_dirty_ = false;

    if (!cache_.tesselator || points_.size() < 3)
    {
        return;
    }

    cache_.coords.clear();
    cache_.coords.reserve(points_.size() * 2);
    for (const auto& pt : points_)
    {
        cache_.coords.push_back(pt.x());
        cache_.coords.push_back(pt.y());
    }

    tessAddContour(cache_.tesselator, 2, cache_.coords.data(), sizeof(float) * 2, static_cast<int>(points_.size()));
    if (tessTesselate(cache_.tesselator, TESS_WINDING_NONZERO, TESS_POLYGONS, 3, 2, nullptr))
    {
        const float* verts  = tessGetVertices(cache_.tesselator);
        const int*   elems  = tessGetElements(cache_.tesselator);
        const int    nelems = tessGetElementCount(cache_.tesselator);

        cache_.vertex.reserve(nelems);

        for (int i = 0; i < nelems; ++i)
        {
            const int* tri = &elems[i * 3];
            if (tri[0] != TESS_UNDEF && tri[1] != TESS_UNDEF && tri[2] != TESS_UNDEF)
            {
                Vertex vertex{};

                vertex.x0.setX(verts[tri[0] * 2]);
                vertex.x0.setY(verts[tri[0] * 2 + 1]);

                vertex.x1.setX(verts[tri[1] * 2]);
                vertex.x1.setY(verts[tri[1] * 2 + 1]);

                vertex.x2.setX(verts[tri[2] * 2]);
                vertex.x2.setY(verts[tri[2] * 2 + 1]);

                cache_.vertex.push_back(vertex);
            }
        }
    }
}

inline const std::list<Vec2F>& PolygonComponent::points() const
{
    return points_;
}

inline const std::vector<Vertex>& PolygonComponent::vertex()
{
    if (cache_dirty_)
    {
        update();
    }
    return cache_.vertex;
}

} // namespace PolygonHuiny

} // namespace Kabuto
