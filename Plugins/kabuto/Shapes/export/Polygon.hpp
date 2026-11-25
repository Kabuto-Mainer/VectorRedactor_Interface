#pragma once

#include "Components.hpp"
#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/IInputPump.hpp"
#include "Plugins/kabuto/SDL3Engine/export/VertexRenderer.hpp"
#include "PolygonComponent.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/Matrix3F.hpp"
#include "Utils/Vec2F.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <istream>
#include <iterator>
#include <limits>
#include <list>
#include <memory>
#include <ostream>
#include <utility>

namespace Kabuto
{

namespace Shapes
{

class PolygonShape final : public IDocument::TShape, public Document::ColorComponent, public PolygonComponent
{
public:
    static constexpr IDocument::TShape::TypeID POLYGON_SHAPE_ID = 1'764;

    explicit PolygonShape(std::list<Vec2F> points = {});
    ~PolygonShape() override;

    static std::unique_ptr<IDocument::TShape> loadFrom(std::istream& stream);
    void                                      saveTo(std::ostream& stream) const;

    IDocument::TShape::Error queryComponent(IDocument::AShapeComponent::ID coid,
                                            IDocument::AShapeComponent const** component) const override;
    IDocument::TShape::Error queryComponent(IDocument::AShapeComponent::ID coid,
                                            IDocument::AShapeComponent** component) override;

    bool                      hitTest(Vec2F const& pos) const override;
    BoxF                      axesAlignedBoundingBox() const override;
    IDocument::TShape::TypeID tid() const override;

    bool movePoint(std::size_t logic_idx, const Vec2F& new_pos) override;

    bool processEvent(const IInputPump::Event& event) override;
    void render() override;
}; // class PolygonShape

/* ====================== Implementation ====================== */

inline PolygonShape::PolygonShape(std::list<Vec2F> points) : PolygonComponent(std::move(points))
{
}

inline PolygonShape::~PolygonShape() = default;

inline std::unique_ptr<IDocument::TShape> PolygonShape::loadFrom(std::istream& stream)
{
    constexpr std::size_t MAX_POINTS = 1'000'000;
    constexpr std::size_t MIN_POINTS = 3;

    float scaling_x{};
    float scaling_y{};
    float rotation{};
    float translation_x{};
    float translation_y{};
    int   red{};
    int   green{};
    int   blue{};
    int   alpha{};
    std::size_t point_count{};

    if (!(stream >>
          scaling_x >>
          scaling_y >>
          rotation >>
          translation_x >>
          translation_y >>
          red >>
          green >>
          blue >>
          alpha >>
          point_count))
    {
        return nullptr;
    }

    if (point_count < MIN_POINTS || point_count > MAX_POINTS)
    {
        return nullptr;
    }

    std::list<Vec2F> points;
    for (std::size_t i = 0; i < point_count; ++i)
    {
        float x{};
        float y{};

        if (!(stream >> x >> y))
        {
            return nullptr;
        }

        points.emplace_back(x, y);
    }

    auto shape = std::make_unique<PolygonShape>(std::move(points));
    shape->setScaling(Vec2F{scaling_x, scaling_y});
    shape->setRotation(rotation);
    shape->setTranslation(Vec2F{translation_x, translation_y});
    shape->setColor(IRenderer::Color{static_cast<uint8_t>(std::clamp(red, 0, 255)),
                                     static_cast<uint8_t>(std::clamp(green, 0, 255)),
                                     static_cast<uint8_t>(std::clamp(blue, 0, 255)),
                                     static_cast<uint8_t>(std::clamp(alpha, 0, 255))});

    return shape;
}

inline void PolygonShape::saveTo(std::ostream& stream) const
{
    stream << std::setprecision(std::numeric_limits<float>::max_digits10) << scaling().x() << ' ' << scaling().y()
           << ' ' << rotation() << ' ' << translation().x() << ' ' << translation().y() << ' '
           << static_cast<unsigned int>(color().r) << ' ' << static_cast<unsigned int>(color().g) << ' '
           << static_cast<unsigned int>(color().b) << ' ' << static_cast<unsigned int>(color().a) << ' '
           << points().size();

    for (const Vec2F& point : points())
    {
        stream << ' ' << point.x() << ' ' << point.y();
    }
}

inline bool PolygonShape::hitTest(Vec2F const& pos) const
{
    Vec2F const local_pos = mapGlobalToLocal(pos);

    for (auto const& triangle : vertex())
    {
        if (triangle.hitTest(local_pos))
        {
            return true;
        }
    }

    return false;
}

inline BoxF PolygonShape::axesAlignedBoundingBox() const
{
    if (points().empty())
    {
        return BoxF{};
    }

    float min_x = std::numeric_limits<float>::infinity();
    float max_x = -std::numeric_limits<float>::infinity();
    float min_y = std::numeric_limits<float>::infinity();
    float max_y = -std::numeric_limits<float>::infinity();

    for (auto const& point : points())
    {
        Vec2F const transformed_point = transform().transformPoint(point);

        min_x = std::min(min_x, transformed_point.x());
        max_x = std::max(max_x, transformed_point.x());

        min_y = std::min(min_y, transformed_point.y());
        max_y = std::max(max_y, transformed_point.y());
    }

    return BoxF{min_x, min_y, max_x, max_y};
}

inline IDocument::TShape::TypeID PolygonShape::tid() const
{
    return POLYGON_SHAPE_ID;
}

inline bool PolygonShape::movePoint(std::size_t logic_idx, const Vec2F& new_pos)
{
    if (logic_idx >= points_.size())
    {
        return false;
    }

    auto it = points_.begin();
    std::advance(it, static_cast<std::ptrdiff_t>(logic_idx));
    *it = new_pos;

    update();
    return true;
}

inline bool PolygonShape::processEvent(IInputPump::Event const& /*event*/)
{
    ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "PolygonShape can't process events\n");
    return false;
}

inline void PolygonShape::render()
{
    IVertexRenderer* renderer{};
    const ApplicationCore::Error error =
        ApplicationCore::instance().queryInterface(IID_VERTEX_RENDERER, reinterpret_cast<void**>(&renderer));

    if (error != ApplicationCore::Error::SUCCESS || renderer == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "PolygonShape can't find the vertex renderer\n");
        return;
    }

    for (auto const& triangle : vertex())
    {
        renderer->renderVertex(color(),
                               IVertexRenderer::Vertex{
                                   mapLocalToGlobal(triangle.x0),
                                   mapLocalToGlobal(triangle.x1),
                                   mapLocalToGlobal(triangle.x2)
        });
    }

    assert(children().empty());
}

inline IDocument::TShape::Error PolygonShape::queryComponent(IDocument::AShapeComponent::ID coid,
                                                             IDocument::AShapeComponent**       component)
{
    if (component == nullptr)
    {
        return IDocument::TShape::Error::INVALID_ARGS;
    }

    switch (coid)
    {
        case PolygonComponent::COID:
        {
            *component = static_cast<PolygonComponent*>(this);
            return IDocument::TShape::Error::SUCCESS;
        }
        case Document::ColorComponent::COID:
        {
            *component = static_cast<Document::ColorComponent*>(this);
            return IDocument::TShape::Error::SUCCESS;
        }
        default:
            break;
    }

    *component = nullptr;
    return IDocument::TShape::Error::COMPONENT_NOT_FOUND;
}

inline IDocument::TShape::Error PolygonShape::queryComponent(IDocument::AShapeComponent::ID     coid,
                                                             IDocument::AShapeComponent const** component) const
{
    if (component == nullptr)
    {
        return IDocument::TShape::Error::INVALID_ARGS;
    }

    switch (coid)
    {
        case PolygonComponent::COID:
        {
            *component = static_cast<PolygonComponent const*>(this);
            return IDocument::TShape::Error::SUCCESS;
        }
        case Document::ColorComponent::COID:
        {
            *component = static_cast<Document::ColorComponent const*>(this);
            return IDocument::TShape::Error::SUCCESS;
        }
        default:
            break;
    }

    *component = nullptr;
    return IDocument::TShape::Error::COMPONENT_NOT_FOUND;
}

} // namespace Shapes

} // namespace Kabuto
