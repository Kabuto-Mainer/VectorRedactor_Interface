#pragma once

#include "Interfaces/IDocument.hpp"
#include "Interfaces/IRenderer.hpp"
#include "ShapeLookup.hpp"
#include "ToolsConfig.hpp"
#include "Utils/TObject.hpp"
#include "Utils/Vec2F.hpp"
#include "export/EllipseShape.hpp"
#include "export/PolygonComponent.hpp"

#include <cstddef>
#include <list>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace Kabuto
{

namespace Tools
{

/**
 * Draws the vertex handles of the edited polygon.
 *
 * The layer is a plain TObject added to the document node, so it is rendered
 * together with the shapes but is not visited by the document hit-test or by
 * the shape factory serialization. Every handle is a regular EllipseShape in
 * the object tree; the layer only keeps their screen size constant and updates
 * their positions from the edited polygon.
 */
class PolygonEditOverlay final : public TObject
{
public:
    PolygonEditOverlay(IDocument& document, TObject::ID shape_id, OverlaySettings settings = {});
    ~PolygonEditOverlay() override = default;

    PolygonEditOverlay(const PolygonEditOverlay&)            = delete;
    PolygonEditOverlay(PolygonEditOverlay&&)                 = delete;
    PolygonEditOverlay& operator=(const PolygonEditOverlay&) = delete;
    PolygonEditOverlay& operator=(PolygonEditOverlay&&)      = delete;

    bool  processEvent(const IInputPump::Event& event) override;
    void  render() override;
    Vec2F mapLocalToGlobal(const Vec2F& pos) const override;
    Vec2F mapGlobalToLocal(const Vec2F& pos) const override;

    void        setSelectedVertex(std::optional<std::size_t> index);
    TObject::ID shapeId() const;

private:
    static constexpr float MIN_SCALE = 1e-6F;

    void rebuildHandles(std::size_t count);

    IDocument&                         document_;
    TObject::ID                        shape_id_;
    OverlaySettings                    settings_;
    std::optional<std::size_t>         selected_vertex_;
    std::vector<Shapes::EllipseShape*> handles_;
}; // class PolygonEditOverlay

/* ====================== Implementation ====================== */

inline PolygonEditOverlay::PolygonEditOverlay(IDocument& document, TObject::ID shape_id, OverlaySettings settings) :
    document_(document), shape_id_(shape_id), settings_(settings)
{
}

inline bool PolygonEditOverlay::processEvent(const IInputPump::Event& /*event*/)
{
    return false;
}

inline void PolygonEditOverlay::render()
{
    FindShapeVisitor visitor(shape_id_);
    document_.accept(visitor);

    IDocument::TShape* shape = visitor.found();
    if (shape == nullptr)
    {
        return;
    }

    IDocument::AShapeComponent* component = nullptr;
    if (shape->queryComponent(Shapes::PolygonComponent::COID, &component) != IDocument::TShape::Error::SUCCESS ||
        component == nullptr)
    {
        return;
    }

    const std::list<Vec2F>& points = static_cast<Shapes::PolygonComponent*>(component)->points();
    if (points.size() != handles_.size())
    {
        rebuildHandles(points.size());
    }

    const Vec2F origin = mapLocalToGlobal(Vec2F{});
    const Vec2F x_axis = mapLocalToGlobal(Vec2F{1.0F, 0.0F}) - origin;
    const Vec2F y_axis = mapLocalToGlobal(Vec2F{0.0F, 1.0F}) - origin;

    const float scale_x = x_axis.len();
    const float scale_y = y_axis.len();
    if (scale_x <= MIN_SCALE || scale_y <= MIN_SCALE)
    {
        return;
    }

    const Vec2F handle_scaling{2.0F * settings_.handle_radius / scale_x, 2.0F * settings_.handle_radius / scale_y};

    std::size_t index = 0;
    for (const Vec2F& point : points)
    {
        Shapes::EllipseShape* handle = handles_[index];

        handle->setTranslation(shape->transform().transformPoint(point));
        handle->setScaling(handle_scaling);
        handle->setColor(selected_vertex_.has_value() && *selected_vertex_ == index ? settings_.selected_color :
                                                                                      settings_.handle_color);

        ++index;
    }

    renderChildren();
}

inline Vec2F PolygonEditOverlay::mapLocalToGlobal(const Vec2F& pos) const
{
    return parent() != nullptr ? parent()->mapLocalToGlobal(pos) : pos;
}

inline Vec2F PolygonEditOverlay::mapGlobalToLocal(const Vec2F& pos) const
{
    return parent() != nullptr ? parent()->mapGlobalToLocal(pos) : pos;
}

inline void PolygonEditOverlay::setSelectedVertex(std::optional<std::size_t> index)
{
    selected_vertex_ = index;
}

inline TObject::ID PolygonEditOverlay::shapeId() const
{
    return shape_id_;
}

inline void PolygonEditOverlay::rebuildHandles(std::size_t count)
{
    children().clear();
    handles_.clear();
    handles_.reserve(count);

    for (std::size_t index = 0; index < count; ++index)
    {
        auto handle = std::make_unique<Shapes::EllipseShape>();
        handle->setColor(settings_.handle_color);

        Shapes::EllipseShape* raw = handle.get();
        addChild(std::move(handle));
        handles_.push_back(raw);
    }
}

}; // namespace Tools

}; // namespace Kabuto
