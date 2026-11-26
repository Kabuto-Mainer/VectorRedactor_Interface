#pragma once

#include "Interfaces/IDocument.hpp"
#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IToolManager.hpp"
#include "ShapeLookup.hpp"
#include "ToolsConfig.hpp"
#include "Utils/TObject.hpp"
#include "Utils/Vec2F.hpp"
#include "export/Components.hpp"
#include "export/ICamera.hpp"
#include "export/IPalette.hpp"
#include "export/Polygon.hpp"
#include "export/PolygonComponent.hpp"

#include <chrono>
#include <cstddef>
#include <list>
#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace Kabuto
{

namespace Tools
{

/**
 * Creates a polygon vertex by vertex.
 *
 * A left click adds a vertex in document coordinates. After the third vertex
 * the shape is inserted into the document, so every following click updates
 * the already visible polygon. The shape is finalized by a right click, Enter,
 * a double click or a click on the first vertex. Escape removes the draft and
 * Backspace removes the last vertex.
 *
 * Vertices are kept relative to their centre, while the centre is stored as
 * the shape translation, so scaling and rotation of the final shape behave the
 * same way as for the other shapes.
 */
class AddPolygonTool final : public IToolManager::ATool
{
public:
    static constexpr IToolManager::ATool::ID ID = 1'324;

    AddPolygonTool(IDocument& document, IPalette& palette, ICamera& camera, AddPolygonSettings settings = {});
    ~AddPolygonTool() override = default;

    AddPolygonTool(const AddPolygonTool&)            = delete;
    AddPolygonTool(AddPolygonTool&&)                 = delete;
    AddPolygonTool& operator=(const AddPolygonTool&) = delete;
    AddPolygonTool& operator=(AddPolygonTool&&)      = delete;

    IToolManager::ATool::ID id() const override;
    bool                    processEvent(const IInputPump::Event& event) override;
    /**
     * Removes the unfinished polygon from the document. Called by the plugin
     * when another tool becomes active.
     */
    void deactivate();

private:
    using Clock = std::chrono::steady_clock;

    static constexpr std::size_t MIN_POINTS = 3;

    bool processMouse(const IInputPump::EventMouse& mouse);
    bool processKeyboard(const IInputPump::EventKeyboard& keyboard);

    bool addPoint(const Vec2F& global_pos);
    bool finishPolygon();
    void cancelPolygon();
    void removeDraft();
    void rebuildDraft();
    void applyPaletteColor(IDocument::TShape& shape) const;

    IDocument::TShape* draftShape();
    Vec2F              pointsCenter() const;
    bool               isNearFirstPoint(const Vec2F& global_pos);

    IDocument&         document_;
    IPalette&          palette_;
    ICamera&           camera_;
    AddPolygonSettings settings_;

    std::vector<Vec2F> points_;

    TObject::ID draft_id_ = 0;

    std::optional<Clock::time_point> last_click_time_;
    std::optional<Vec2F>             last_click_pos_;
    std::optional<Vec2F>             last_added_global_;
}; // class AddPolygonTool

/* ====================== Implementation ====================== */

inline AddPolygonTool::AddPolygonTool(IDocument& document, IPalette& palette, ICamera& camera,
                                      AddPolygonSettings settings) :
    document_(document), palette_(palette), camera_(camera), settings_(settings)
{
}

inline IToolManager::ATool::ID AddPolygonTool::id() const
{
    return ID;
}

inline void AddPolygonTool::deactivate()
{
    cancelPolygon();
}

inline bool AddPolygonTool::processEvent(const IInputPump::Event& event)
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

inline bool AddPolygonTool::processMouse(const IInputPump::EventMouse& mouse)
{
    using enum IInputPump::EventMouse::Type;

    switch (mouse.type())
    {
        case LEFT_BUTTON_PRESSING:
            return addPoint(mouse.cursorPos());

        case RIGHT_BUTTON_PRESSING:
            if (points_.empty())
            {
                return false;
            }

            if (points_.size() < MIN_POINTS)
            {
                cancelPolygon();
                return true;
            }

            return finishPolygon();

        case MOTION:
            /* While a polygon is being drawn, the tool owns the canvas. */
            return !points_.empty();

        default:
            return false;
    }
}

inline bool AddPolygonTool::processKeyboard(const IInputPump::EventKeyboard& keyboard)
{
    if (keyboard.type() != IInputPump::EventKeyboard::Type::KEY_DOWN)
    {
        return false;
    }

    using enum IInputPump::EventKeyboard::Key;

    switch (keyboard.key())
    {
        case ESC:
            if (points_.empty())
            {
                return false;
            }

            cancelPolygon();
            return true;

        case ENTER:
            if (points_.empty())
            {
                return false;
            }

            return finishPolygon();

        case BACKSPACE:
            if (points_.empty())
            {
                return false;
            }

            points_.pop_back();
            last_added_global_.reset();
            rebuildDraft();
            return true;

        default:
            return false;
    }
}

inline bool AddPolygonTool::addPoint(const Vec2F& global_pos)
{
    const Clock::time_point now = Clock::now();

    const bool double_click =
        points_.size() >= MIN_POINTS &&
        last_click_time_.has_value() &&
        last_click_pos_.has_value() &&
        std::chrono::duration<float>(now - *last_click_time_).count() <= settings_.double_click_seconds &&
        (global_pos - *last_click_pos_).len2() <= settings_.double_click_distance * settings_.double_click_distance;

    last_click_time_ = now;
    last_click_pos_  = global_pos;

    if (double_click)
    {
        return finishPolygon();
    }

    if (isNearFirstPoint(global_pos))
    {
        return finishPolygon();
    }

    /* A repeated click in the same place must not create a zero-length edge. */
    if (last_added_global_.has_value() &&
        (global_pos - *last_added_global_).len2() <= settings_.duplicate_distance * settings_.duplicate_distance)
    {
        return true;
    }

    points_.push_back(camera_.mapGlobalToLocal(global_pos));
    last_added_global_ = global_pos;
    rebuildDraft();

    return true;
}

inline bool AddPolygonTool::finishPolygon()
{
    if (points_.size() < MIN_POINTS)
    {
        cancelPolygon();
        return true;
    }

    /* The draft may have been removed by another tool while this one was
       inactive; recreate it so that finishing always leaves a shape. */
    if (draftShape() == nullptr)
    {
        rebuildDraft();
    }

    /* The draft shape now owns its points; the tool must not remove it. */
    draft_id_ = 0;
    points_.clear();
    last_added_global_.reset();

    return true;
}

inline void AddPolygonTool::cancelPolygon()
{
    removeDraft();
    points_.clear();
    last_click_time_.reset();
    last_click_pos_.reset();
    last_added_global_.reset();
}

inline void AddPolygonTool::removeDraft()
{
    if (draft_id_ != 0)
    {
        document_.removeShape(draft_id_);
    }

    draft_id_ = 0;
}

inline void AddPolygonTool::rebuildDraft()
{
    if (points_.size() < MIN_POINTS)
    {
        removeDraft();
        return;
    }

    const Vec2F center = pointsCenter();

    std::list<Vec2F> local_points;
    for (const Vec2F& point : points_)
    {
        local_points.push_back(point - center);
    }

    IDocument::TShape* draft = draftShape();
    if (draft == nullptr)
    {
        auto shape = std::make_unique<Shapes::PolygonShape>(std::move(local_points));
        shape->setTranslation(center);
        applyPaletteColor(*shape);

        IDocument::TShape* raw = shape.get();
        draft_id_              = raw->id();
        document_.addShape(std::move(shape));

        return;
    }

    IDocument::AShapeComponent* component = nullptr;
    if (draft->queryComponent(Shapes::PolygonComponent::COID, &component) != IDocument::TShape::Error::SUCCESS ||
        component == nullptr)
    {
        removeDraft();
        return;
    }

    if (!static_cast<Shapes::PolygonComponent*>(component)->setPoints(local_points))
    {
        removeDraft();
        return;
    }

    draft->setTranslation(center);
}

inline IDocument::TShape* AddPolygonTool::draftShape()
{
    if (draft_id_ == 0)
    {
        return nullptr;
    }

    FindShapeVisitor visitor(draft_id_);
    document_.accept(visitor);

    IDocument::TShape* shape = visitor.found();
    if (shape == nullptr)
    {
        draft_id_ = 0;
    }

    return shape;
}

inline void AddPolygonTool::applyPaletteColor(IDocument::TShape& shape) const
{
    IDocument::AShapeComponent* component = nullptr;
    if (shape.queryComponent(Document::ColorComponent::COID, &component) == IDocument::TShape::Error::SUCCESS &&
        component != nullptr)
    {
        static_cast<Document::ColorComponent*>(component)->setColor(palette_.color());
    }
}

inline Vec2F AddPolygonTool::pointsCenter() const
{
    if (points_.empty())
    {
        return {};
    }

    Vec2F center{};
    for (const Vec2F& point : points_)
    {
        center += point;
    }

    return center / static_cast<float>(points_.size());
}

inline bool AddPolygonTool::isNearFirstPoint(const Vec2F& global_pos)
{
    IDocument::TShape* draft = draftShape();
    if (draft == nullptr || points_.size() < MIN_POINTS)
    {
        return false;
    }

    const Vec2F first_global = draft->mapLocalToGlobal(points_.front() - pointsCenter());

    return (first_global - global_pos).len2() <= settings_.close_distance * settings_.close_distance;
}

}; // namespace Tools

}; // namespace Kabuto
