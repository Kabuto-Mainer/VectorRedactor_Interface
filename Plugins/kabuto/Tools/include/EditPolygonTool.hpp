#pragma once

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IToolManager.hpp"
#include "PolygonEditOverlay.hpp"
#include "ShapeLookup.hpp"
#include "Utils/TObject.hpp"
#include "Utils/Vec2F.hpp"
#include "export/PolygonComponent.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <iterator>
#include <list>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace Kabuto
{

namespace Tools
{

/**
 * Moves, inserts and removes vertices of an existing polygon.
 *
 * A left click selects the topmost polygon. Dragging a vertex moves it; the
 * cursor is converted into the shape-local space, so scaled and rotated shapes
 * are edited correctly. A double click near an edge inserts a vertex, Delete
 * or Backspace removes the last clicked vertex. Escape cancels an active drag
 * and otherwise clears the selection.
 */
class EditPolygonTool final : public IToolManager::ATool
{
public:
    static constexpr IToolManager::ATool::ID ID = 1'325;

    explicit EditPolygonTool(IDocument& document);
    ~EditPolygonTool() override = default;

    EditPolygonTool(const EditPolygonTool&)            = delete;
    EditPolygonTool(EditPolygonTool&&)                 = delete;
    EditPolygonTool& operator=(const EditPolygonTool&) = delete;
    EditPolygonTool& operator=(EditPolygonTool&&)      = delete;

    IToolManager::ATool::ID id() const override;
    bool                    processEvent(const IInputPump::Event& event) override;
    /**
     * Cancels an unfinished vertex drag and removes the vertex handle overlay
     * from the object tree. Called by the plugin when another tool becomes
     * active.
     */
    void deactivate();

private:
    using Clock = std::chrono::steady_clock;

    static constexpr std::size_t MIN_POINTS            = 3;
    static constexpr float       VERTEX_PICK_RADIUS    = 10.0F;
    static constexpr float       EDGE_PICK_RADIUS      = 8.0F;
    static constexpr float       DOUBLE_CLICK_DISTANCE = 8.0F;
    static constexpr float       DOUBLE_CLICK_SECONDS  = 0.35F;
    static constexpr float       EDGE_EPSILON          = 1e-6F;

    bool processMouse(const IInputPump::EventMouse& mouse);
    bool processKeyboard(const IInputPump::EventKeyboard& keyboard);

    bool press(const Vec2F& global_pos);
    bool move(const Vec2F& global_pos);
    bool release(const Vec2F& global_pos);
    bool cancel();

    IDocument::TShape*        selectedShape();
    Shapes::PolygonComponent* selectedPolygon();
    void                      resetSelection();

    PolygonEditOverlay* overlay();
    void                showOverlay(IDocument::TShape& shape);
    void                hideOverlay();
    void                refreshOverlay();

    std::optional<std::size_t> nearestVertex(IDocument::TShape& shape, const Shapes::PolygonComponent& polygon,
                                             const Vec2F& global_pos) const;
    bool                       beginDrag(const Shapes::PolygonComponent& polygon, std::size_t index);
    bool insertVertex(IDocument::TShape& shape, Shapes::PolygonComponent& polygon, const Vec2F& global_pos);
    bool deleteSelectedVertex();

    IDocument& document_;

    std::optional<TObject::ID> selected_id_;
    std::optional<std::size_t> selected_vertex_;
    std::optional<std::size_t> dragged_vertex_;
    Vec2F                      drag_origin_;
    TObject::ID                overlay_id_ = 0;

    std::optional<Clock::time_point> last_press_time_;
    std::optional<Vec2F>             last_press_pos_;
}; // class EditPolygonTool

/* ====================== Implementation ====================== */

inline EditPolygonTool::EditPolygonTool(IDocument& document) : document_(document)
{
}

inline IToolManager::ATool::ID EditPolygonTool::id() const
{
    return ID;
}

inline void EditPolygonTool::deactivate()
{
    if (dragged_vertex_.has_value())
    {
        cancel();
    }

    resetSelection();
}

inline bool EditPolygonTool::processEvent(const IInputPump::Event& event)
{
    if (const auto* keyboard = std::get_if<IInputPump::EventKeyboard>(&event))
    {
        return processKeyboard(*keyboard);
    }

    if (const auto* mouse = std::get_if<IInputPump::EventMouse>(&event))
    {
        return processMouse(*mouse);
    }

    return false;
}

inline bool EditPolygonTool::processMouse(const IInputPump::EventMouse& mouse)
{
    using enum IInputPump::EventMouse::Type;

    switch (mouse.type())
    {
        case LEFT_BUTTON_PRESSING:
            return press(mouse.cursorPos());

        case LEFT_BUTTON_RELEASING:
            return release(mouse.cursorPos());

        case MOTION:
            return move(mouse.cursorPos());

        case RIGHT_BUTTON_PRESSING:
            return cancel();

        default:
            return false;
    }
}

inline bool EditPolygonTool::processKeyboard(const IInputPump::EventKeyboard& keyboard)
{
    if (keyboard.type() != IInputPump::EventKeyboard::Type::KEY_DOWN)
    {
        return false;
    }

    using enum IInputPump::EventKeyboard::Key;

    switch (keyboard.key())
    {
        case ESC:
            return cancel();

        case DEL:
        case BACKSPACE:
            return deleteSelectedVertex();

        default:
            return false;
    }
}

inline bool EditPolygonTool::press(const Vec2F& global_pos)
{
    const Clock::time_point now = Clock::now();

    const bool double_click = last_press_time_.has_value() &&
                              last_press_pos_.has_value() &&
                              std::chrono::duration<float>(now - *last_press_time_).count() <= DOUBLE_CLICK_SECONDS &&
                              (global_pos - *last_press_pos_).len2() <= DOUBLE_CLICK_DISTANCE * DOUBLE_CLICK_DISTANCE;

    last_press_time_ = now;
    last_press_pos_  = global_pos;

    /* A lost release event must not leave the tool stuck in the dragging state. */
    if (dragged_vertex_.has_value())
    {
        release(global_pos);
    }

    IDocument::TShape*        shape   = selectedShape();
    Shapes::PolygonComponent* polygon = selectedPolygon();

    if (shape != nullptr && polygon != nullptr)
    {
        if (const std::optional<std::size_t> vertex = nearestVertex(*shape, *polygon, global_pos); vertex.has_value())
        {
            if (!beginDrag(*polygon, *vertex))
            {
                return false;
            }

            showOverlay(*shape);
            return true;
        }

        if (double_click && insertVertex(*shape, *polygon, global_pos))
        {
            refreshOverlay();
            return true;
        }
    }

    const std::optional<TObject::ID> shape_id = document_.hitTest(global_pos);
    if (!shape_id.has_value())
    {
        resetSelection();
        return true;
    }

    selected_id_ = *shape_id;
    selected_vertex_.reset();

    shape   = selectedShape();
    polygon = selectedPolygon();
    if (shape == nullptr || polygon == nullptr)
    {
        resetSelection();
        return true;
    }

    if (const std::optional<std::size_t> vertex = nearestVertex(*shape, *polygon, global_pos); vertex.has_value())
    {
        if (!beginDrag(*polygon, *vertex))
        {
            return false;
        }

        showOverlay(*shape);
        return true;
    }

    showOverlay(*shape);
    return true;
}

inline bool EditPolygonTool::move(const Vec2F& global_pos)
{
    if (!dragged_vertex_.has_value())
    {
        return false;
    }

    IDocument::TShape* shape = selectedShape();
    if (shape == nullptr)
    {
        dragged_vertex_.reset();
        return false;
    }

    Shapes::PolygonComponent* polygon = selectedPolygon();
    if (polygon == nullptr)
    {
        dragged_vertex_.reset();
        return false;
    }

    if (!polygon->movePoint(*dragged_vertex_, shape->mapGlobalToLocal(global_pos)))
    {
        dragged_vertex_.reset();
        return false;
    }

    return true;
}

inline bool EditPolygonTool::release(const Vec2F& global_pos)
{
    if (!dragged_vertex_.has_value())
    {
        return false;
    }

    move(global_pos);
    dragged_vertex_.reset();

    return true;
}

inline bool EditPolygonTool::cancel()
{
    if (dragged_vertex_.has_value())
    {
        if (IDocument::TShape* shape = selectedShape(); shape != nullptr)
        {
            if (Shapes::PolygonComponent* polygon = selectedPolygon(); polygon != nullptr)
            {
                polygon->movePoint(*dragged_vertex_, drag_origin_);
            }
        }

        dragged_vertex_.reset();
        refreshOverlay();
        return true;
    }

    if (selected_id_.has_value() || selected_vertex_.has_value())
    {
        resetSelection();
        return true;
    }

    return false;
}

inline IDocument::TShape* EditPolygonTool::selectedShape()
{
    if (!selected_id_.has_value())
    {
        return nullptr;
    }

    FindShapeVisitor visitor(*selected_id_);
    document_.accept(visitor);

    IDocument::TShape* shape = visitor.found();
    if (shape == nullptr)
    {
        resetSelection();
    }

    return shape;
}

inline Shapes::PolygonComponent* EditPolygonTool::selectedPolygon()
{
    IDocument::TShape* shape = selectedShape();
    if (shape == nullptr)
    {
        return nullptr;
    }

    IDocument::AShapeComponent* component = nullptr;
    if (shape->queryComponent(Shapes::PolygonComponent::COID, &component) != IDocument::TShape::Error::SUCCESS ||
        component == nullptr)
    {
        resetSelection();
        return nullptr;
    }

    return static_cast<Shapes::PolygonComponent*>(component);
}

inline void EditPolygonTool::resetSelection()
{
    selected_id_.reset();
    selected_vertex_.reset();
    dragged_vertex_.reset();
    hideOverlay();
}

inline PolygonEditOverlay* EditPolygonTool::overlay()
{
    if (overlay_id_ == 0)
    {
        return nullptr;
    }

    TObject* object = ApplicationCore::instance().findObject(overlay_id_);
    return dynamic_cast<PolygonEditOverlay*>(object);
}

inline void EditPolygonTool::showOverlay(IDocument::TShape& shape)
{
    hideOverlay();

    TObject* parent = shape.parent();
    if (parent == nullptr)
    {
        return;
    }

    auto overlay = std::make_unique<PolygonEditOverlay>(document_, shape.id());
    overlay->setSelectedVertex(selected_vertex_);

    PolygonEditOverlay* raw = overlay.get();
    parent->addChild(std::move(overlay));
    overlay_id_ = raw->id();
}

inline void EditPolygonTool::hideOverlay()
{
    if (overlay_id_ != 0 && ApplicationCore::instance().findObject(overlay_id_) != nullptr)
    {
        ApplicationCore::instance().removeObject(overlay_id_);
    }

    overlay_id_ = 0;
}

inline void EditPolygonTool::refreshOverlay()
{
    if (!selected_id_.has_value())
    {
        hideOverlay();
        return;
    }

    IDocument::TShape* shape = selectedShape();
    if (shape == nullptr)
    {
        hideOverlay();
        return;
    }

    PolygonEditOverlay* current = overlay();
    if (current == nullptr || current->shapeId() != shape->id() || current->parent() != shape->parent())
    {
        showOverlay(*shape);
        return;
    }

    current->setSelectedVertex(selected_vertex_);
}

inline std::optional<std::size_t> EditPolygonTool::nearestVertex(IDocument::TShape&              shape,
                                                                 const Shapes::PolygonComponent& polygon,
                                                                 const Vec2F&                    global_pos) const
{
    std::optional<std::size_t> nearest;
    float                      best_distance2 = VERTEX_PICK_RADIUS * VERTEX_PICK_RADIUS;

    std::size_t index = 0;
    for (const Vec2F& point : polygon.points())
    {
        const Vec2F global_point = shape.mapLocalToGlobal(point);
        const float distance2    = (global_point - global_pos).len2();

        if (distance2 <= best_distance2)
        {
            best_distance2 = distance2;
            nearest        = index;
        }

        ++index;
    }

    return nearest;
}

inline bool EditPolygonTool::beginDrag(const Shapes::PolygonComponent& polygon, std::size_t index)
{
    const std::list<Vec2F>& points = polygon.points();
    if (index >= points.size())
    {
        return false;
    }

    auto point = points.begin();
    std::advance(point, static_cast<std::ptrdiff_t>(index));

    if (point == points.end())
    {
        return false;
    }

    dragged_vertex_  = index;
    selected_vertex_ = index;
    drag_origin_     = *point;

    return true;
}

inline bool EditPolygonTool::insertVertex(IDocument::TShape& shape, Shapes::PolygonComponent& polygon,
                                          const Vec2F& global_pos)
{
    const std::list<Vec2F>& points = polygon.points();
    if (points.size() < MIN_POINTS)
    {
        return false;
    }

    std::vector<Vec2F> global_points;
    global_points.reserve(points.size());
    for (const Vec2F& point : points)
    {
        global_points.push_back(shape.mapLocalToGlobal(point));
    }

    float       best_distance2 = EDGE_PICK_RADIUS * EDGE_PICK_RADIUS;
    std::size_t best_after     = 0;
    Vec2F       insert_local{};
    bool        found = false;

    for (std::size_t i = 0; i < global_points.size(); ++i)
    {
        const Vec2F& first  = global_points[i];
        const Vec2F& second = global_points[(i + 1) % global_points.size()];

        const Vec2F edge      = second - first;
        const float edge_len2 = edge.len2();
        if (edge_len2 <= EDGE_EPSILON)
        {
            continue;
        }

        const float t         = std::clamp((global_pos - first).dot(edge) / edge_len2, 0.0F, 1.0F);
        const Vec2F closest   = first + edge * t;
        const float distance2 = (closest - global_pos).len2();

        if (distance2 < best_distance2)
        {
            best_distance2 = distance2;
            best_after     = i;
            insert_local   = shape.mapGlobalToLocal(closest);
            found          = true;
        }
    }

    if (!found)
    {
        return false;
    }

    std::list<Vec2F> updated   = points;
    auto             insert_at = updated.begin();
    std::advance(insert_at, static_cast<std::ptrdiff_t>(best_after + 1));
    updated.insert(insert_at, insert_local);

    if (!polygon.setPoints(updated))
    {
        return false;
    }

    selected_vertex_ = best_after + 1;

    return true;
}

inline bool EditPolygonTool::deleteSelectedVertex()
{
    if (dragged_vertex_.has_value() || !selected_vertex_.has_value())
    {
        return false;
    }

    IDocument::TShape*        shape   = selectedShape();
    Shapes::PolygonComponent* polygon = selectedPolygon();
    if (shape == nullptr || polygon == nullptr)
    {
        return false;
    }

    const std::list<Vec2F>& points = polygon->points();
    if (points.size() <= MIN_POINTS)
    {
        return true;
    }

    if (*selected_vertex_ >= points.size())
    {
        selected_vertex_.reset();
        refreshOverlay();
        return false;
    }

    std::list<Vec2F> updated = points;
    auto             point   = updated.begin();
    std::advance(point, static_cast<std::ptrdiff_t>(*selected_vertex_));

    if (point == updated.end())
    {
        selected_vertex_.reset();
        refreshOverlay();
        return false;
    }

    updated.erase(point);

    if (!polygon->setPoints(updated))
    {
        return true;
    }

    selected_vertex_.reset();
    refreshOverlay();

    return true;
}

}; // namespace Tools

}; // namespace Kabuto
