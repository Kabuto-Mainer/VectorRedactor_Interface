#include "export/PolygonComponent.hpp"

#include <cassert>
#include <cmath>
#include <limits>
#include <utility>

namespace Kabuto
{

namespace Shapes
{

namespace
{

constexpr std::size_t MIN_POINTS = 3;
constexpr float       DEGENERATE_TRIANGLE_EPSILON = 1e-6F;

void resetTesselator(TESStesselator*& tesselator)
{
    if (tesselator != nullptr)
    {
        tessDeleteTess(tesselator);
    }

    tesselator = tessNewTess(nullptr);
}

} // namespace

PolygonComponent::PolygonComponent(std::list<Vec2F> points) : points_(std::move(points))
{
    cache_.tesselator = tessNewTess(nullptr);
    assert(cache_.tesselator != nullptr);

    update();
}

PolygonComponent::~PolygonComponent()
{
    if (cache_.tesselator != nullptr)
    {
        tessDeleteTess(cache_.tesselator);
    }
}

void PolygonComponent::update() const
{
    cache_.vertex.clear();
    cache_dirty_ = false;

    if (cache_.tesselator == nullptr || points_.size() < MIN_POINTS ||
        points_.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
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

    if (!tessTesselate(cache_.tesselator, TESS_WINDING_NONZERO, TESS_POLYGONS, 3, 2, nullptr))
    {
        /* libtess2 keeps its failure status until a new tesselator is created. */
        resetTesselator(cache_.tesselator);
        return;
    }

    const float* verts  = tessGetVertices(cache_.tesselator);
    const int*   elems  = tessGetElements(cache_.tesselator);
    const int    nelems = tessGetElementCount(cache_.tesselator);

    if (verts == nullptr || elems == nullptr || nelems <= 0)
    {
        return;
    }

    cache_.vertex.reserve(static_cast<std::size_t>(nelems));

    for (int i = 0; i < nelems; ++i)
    {
        const int* tri = &elems[i * 3];
        if (tri[0] == TESS_UNDEF || tri[1] == TESS_UNDEF || tri[2] == TESS_UNDEF)
        {
            continue;
        }

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

const std::list<Vec2F>& PolygonComponent::points() const
{
    return points_;
}

bool PolygonComponent::setPoints(const std::list<Vec2F>& points)
{
    if (points.size() < MIN_POINTS)
    {
        return false;
    }

    points_ = points;
    update();
    return true;
}

const std::vector<Vertex>& PolygonComponent::vertex() const
{
    if (cache_dirty_)
    {
        update();
    }

    return cache_.vertex;
}

bool Vertex::hitTest(const Vec2F& point) const
{
    const Vec2F first_edge  = x1 - x0;
    const Vec2F second_edge = x2 - x1;
    const Vec2F third_edge  = x0 - x2;

    const float area = first_edge.dot((x2 - x0).ortogonal());
    if (std::abs(area) <= DEGENERATE_TRIANGLE_EPSILON)
    {
        return false;
    }

    const float c0 = first_edge.dot((point - x0).ortogonal());
    const float c1 = second_edge.dot((point - x1).ortogonal());
    const float c2 = third_edge.dot((point - x2).ortogonal());

    const bool has_negative = (c0 < 0.0F) || (c1 < 0.0F) || (c2 < 0.0F);
    const bool has_positive = (c0 > 0.0F) || (c1 > 0.0F) || (c2 > 0.0F);

    return !(has_negative && has_positive);
}

} // namespace Shapes

} // namespace Kabuto
