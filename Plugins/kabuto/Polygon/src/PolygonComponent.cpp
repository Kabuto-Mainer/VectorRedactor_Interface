#include "export/PolygonComponent.hpp"

namespace Kabuto
{

namespace PolygonHuiny
{

PolygonComponent::PolygonComponent(std::list<Vec2F> points) : points_(points)
{
    cache_.tesselator = tessNewTess(nullptr);
    if (cache_.tesselator == nullptr)
    {
        assert(0);
    }
    update();
}

PolygonComponent::~PolygonComponent()
{
    if (cache_.tesselator)
    {
        tessDeleteTess(cache_.tesselator);
    }
}

void PolygonComponent::update()
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

const std::list<Vec2F>& PolygonComponent::points() const
{
    return points_;
}

const std::vector<Vertex>& PolygonComponent::vertex()
{
    if (cache_dirty_)
    {
        update();
    }
    return cache_.vertex;
}

} // namespace PolygonHuiny

} // namespace Kabuto
