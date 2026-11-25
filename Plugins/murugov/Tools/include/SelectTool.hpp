#pragma once

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/IToolManager.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/Matrix3F.hpp"
#include "Utils/TObject.hpp"
#include "Utils/Vec2F.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace Murugov
{

namespace Tools
{

constexpr IToolManager::ATool::ID SELECT_TOOL_ID = 5;

class SelectTool final : public IToolManager::ATool
{
public:
    SelectTool(ID tool_id, IDocument& document, IRenderer& renderer);
    ~SelectTool() override = default;

    SelectTool(const SelectTool&)            = delete;
    SelectTool(SelectTool&&)                 = delete;
    SelectTool& operator=(const SelectTool&) = delete;
    SelectTool& operator=(SelectTool&&)      = delete;

    ID   id() const override;
    bool processEvent(const IInputPump::Event& event) override;
    void render();

private:
    enum class Mode
    {
        NONE,
        MOVE,
        RESIZE,
        ROTATE,
        BOX_SELECT,
    };

    class ShapeCollector;

    struct InitialShapeState
    {
        Vec2F translation;
        Vec2F scaling;
        float rotation = 0.0F;
    };

    static constexpr float HANDLE_HALF_SIZE   = 4.5F;
    static constexpr float ROTATE_HANDLE_DIST = 26.0F;
    static constexpr float MIN_SHAPE_SIZE     = 4.0F;
    static constexpr float MIN_MARQUEE_SIZE   = 2.0F;

    Vec2F toLocal(const Vec2F& global_pos) const;
    Vec2F toGlobal(const Vec2F& local_pos) const;

    /**
     * Converts a screen-space radius into the document-local units used by
     * hit tests, so handles keep the same grab area at any camera zoom.
     */
    float toLocalRadius(float screen_radius, const Vec2F& local_anchor) const;

    /**
     * Returns the node in whose coordinate space the shapes live.
     *
     * IDocument is often a proxy (Kabuto's CDocument is not a TObject), so the
     * document itself cannot always be used for mapGlobalToLocal(). Every
     * shape, however, has the real document node as its parent. That node is
     * the correct frame for both shape transforms and axes-aligned bounds.
     */
    const TObject* documentNode() const;

    /**
     * True for shapes whose geometry is the unit square [-0.5; 0.5] scaled by
     * Transformable (rectangles, ellipses, ...). Polygons store arbitrary
     * local points, so they need generic AABB handles instead.
     */
    bool usesUnitSquareFrame(const IDocument::TShape& shape) const;

    std::vector<IDocument::TShape*> selectedShapes() const;
    bool                            isSelected(TObject::ID id) const;
    void                            clearSelection();
    void                            captureInitialStates(const std::vector<IDocument::TShape*>& shapes);

    std::optional<BoxF>  groupBounds(const std::vector<IDocument::TShape*>& shapes) const;
    std::array<Vec2F, 8> selectionHandles(const std::vector<IDocument::TShape*>& shapes) const;
    Vec2F                rotateHandlePos(const std::vector<IDocument::TShape*>& shapes) const;

    bool hitTestPoint(const Vec2F& p, const Vec2F& target, float radius) const;
    int  hitTestResizeHandle(const std::vector<IDocument::TShape*>& shapes, const Vec2F& local_pos) const;

    bool onMouseDown(const Vec2F& global_pos);
    bool onMouseMove(const Vec2F& global_pos);
    bool onMouseUp();
    bool onKeyDown(const IInputPump::EventKeyboard& key);

private:
    ID         id_;
    IDocument& document_;
    IRenderer& renderer_;

    mutable const TObject* document_node_ = nullptr;

    std::unordered_set<TObject::ID> selected_ids_;
    Mode                            mode_ = Mode::NONE;

    // For the selection marquee
    Vec2F box_select_start_;
    Vec2F box_select_current_;

    // For transforming a single figure or a group
    Vec2F                                              prev_mouse_local_;
    Vec2F                                              fixed_anchor_;
    Vec2F                                              group_center_;
    BoxF                                               initial_group_box_;
    float                                              group_visual_angle_ = 0.0F;
    int                                                active_handle_idx_  = -1;
    std::unordered_map<TObject::ID, InitialShapeState> initial_states_;
}; // class SelectTool

class SelectTool::ShapeCollector final : public IDocument::Visitor
{
public:
    bool visit(IDocument::TShape& shape) override
    {
        shapes_.push_back(&shape);
        return true;
    }

    bool visit(const IDocument::TShape& shape) override
    {
        shapes_.push_back(&shape);
        return true;
    }

    const std::vector<const IDocument::TShape*>& shapes() const
    {
        return shapes_;
    }

private:
    std::vector<const IDocument::TShape*> shapes_;
}; // class SelectTool::ShapeCollector

/* ====================== Implementation ====================== */

inline SelectTool::SelectTool(ID tool_id, IDocument& document, IRenderer& renderer) :
    id_(tool_id), document_(document), renderer_(renderer)
{
}

inline IToolManager::ATool::ID SelectTool::id() const
{
    return id_;
}

inline const TObject* SelectTool::documentNode() const
{
    if (document_node_ != nullptr)
    {
        return document_node_;
    }

    class NodeCollector final : public IDocument::Visitor
    {
    public:
        bool visit(IDocument::TShape& shape) override
        {
            return visit(static_cast<const IDocument::TShape&>(shape));
        }

        bool visit(const IDocument::TShape& shape) override
        {
            node = shape.parent();
            return false;
        }

        const TObject* node = nullptr;
    } collector;

    document_.accept(collector);
    document_node_ = collector.node;
    return document_node_;
}

inline Vec2F SelectTool::toLocal(const Vec2F& global_pos) const
{
    const TObject* node = documentNode();
    return node != nullptr ? node->mapGlobalToLocal(global_pos) : global_pos;
}

inline Vec2F SelectTool::toGlobal(const Vec2F& local_pos) const
{
    const TObject* node = documentNode();
    return node != nullptr ? node->mapLocalToGlobal(local_pos) : local_pos;
}

inline float SelectTool::toLocalRadius(float screen_radius, const Vec2F& local_anchor) const
{
    const TObject* node = documentNode();
    if (node == nullptr)
    {
        return screen_radius;
    }

    const Vec2F global = node->mapLocalToGlobal(local_anchor);
    const Vec2F dx     = node->mapGlobalToLocal(global + Vec2F{screen_radius, 0.0F}) - local_anchor;
    const Vec2F dy     = node->mapGlobalToLocal(global + Vec2F{0.0F, screen_radius}) - local_anchor;
    return std::max(dx.len(), dy.len());
}

inline bool SelectTool::usesUnitSquareFrame(const IDocument::TShape& shape) const
{
    const BoxF  bounds = shape.axesAlignedBoundingBox();
    const float left   = std::min(bounds.left(), bounds.right());
    const float right  = std::max(bounds.left(), bounds.right());
    const float top    = std::min(bounds.top(), bounds.bottom());
    const float bottom = std::max(bounds.top(), bounds.bottom());

    const Matrix3F& matrix = shape.transform();

    const Vec2F unit_corners[] = {
        matrix.transformPoint({-0.5F, -0.5F}),
        matrix.transformPoint({-0.5F, +0.5F}),
        matrix.transformPoint({+0.5F, -0.5F}),
        matrix.transformPoint({+0.5F, +0.5F}),
    };
    const float unit_left =
        std::min({unit_corners[0].x(), unit_corners[1].x(), unit_corners[2].x(), unit_corners[3].x()});
    const float unit_right =
        std::max({unit_corners[0].x(), unit_corners[1].x(), unit_corners[2].x(), unit_corners[3].x()});
    const float unit_top =
        std::min({unit_corners[0].y(), unit_corners[1].y(), unit_corners[2].y(), unit_corners[3].y()});
    const float unit_bottom =
        std::max({unit_corners[0].y(), unit_corners[1].y(), unit_corners[2].y(), unit_corners[3].y()});

    /* An ellipse reports the exact support box of its rotated radii rather
       than the box of the enclosing unit square. */
    const Vec2F center         = shape.translation();
    const float extent_x       = 0.5F * std::hypot(matrix[0][0], matrix[0][1]);
    const float extent_y       = 0.5F * std::hypot(matrix[1][0], matrix[1][1]);
    const float ellipse_left   = center.x() - extent_x;
    const float ellipse_right  = center.x() + extent_x;
    const float ellipse_top    = center.y() - extent_y;
    const float ellipse_bottom = center.y() + extent_y;

    const float scale = std::max({std::abs(left), std::abs(right), std::abs(top), std::abs(bottom), 1.0F});
    const float eps   = 1e-3F * scale;

    const auto matches = [eps](float a, float b) { return std::abs(a - b) <= eps; };

    const bool unit_square = matches(left, unit_left) &&
                             matches(right, unit_right) &&
                             matches(top, unit_top) &&
                             matches(bottom, unit_bottom);
    const bool ellipse     = matches(left, ellipse_left) &&
                             matches(right, ellipse_right) &&
                             matches(top, ellipse_top) &&
                             matches(bottom, ellipse_bottom);

    return unit_square || ellipse;
}

inline std::vector<IDocument::TShape*> SelectTool::selectedShapes() const
{
    std::vector<IDocument::TShape*> result;
    result.reserve(selected_ids_.size());

    for (const TObject::ID shape_id : selected_ids_)
    {
        auto* shape = dynamic_cast<IDocument::TShape*>(ApplicationCore::instance().findObject(shape_id));
        if (shape != nullptr)
        {
            result.push_back(shape);
        }
    }

    return result;
}

inline bool SelectTool::isSelected(TObject::ID id) const
{
    return selected_ids_.contains(id);
}

inline void SelectTool::clearSelection()
{
    selected_ids_.clear();
    initial_states_.clear();
    mode_               = Mode::NONE;
    active_handle_idx_  = -1;
    group_visual_angle_ = 0.0F;
}

inline void SelectTool::captureInitialStates(const std::vector<IDocument::TShape*>& shapes)
{
    initial_states_.clear();
    for (const auto* shape : shapes)
    {
        initial_states_[shape->id()] = InitialShapeState{
            .translation = shape->translation(),
            .scaling     = shape->scaling(),
            .rotation    = shape->rotation(),
        };
    }
}

inline std::optional<BoxF> SelectTool::groupBounds(const std::vector<IDocument::TShape*>& shapes) const
{
    if (shapes.empty())
    {
        return std::nullopt;
    }

    BoxF  first = shapes.front()->axesAlignedBoundingBox();
    float min_x = std::min(first.left(), first.right());
    float min_y = std::min(first.top(), first.bottom());
    float max_x = std::max(first.left(), first.right());
    float max_y = std::max(first.top(), first.bottom());

    for (std::size_t i = 1; i < shapes.size(); ++i)
    {
        const BoxF b = shapes[i]->axesAlignedBoundingBox();
        min_x        = std::min({min_x, b.left(), b.right()});
        min_y        = std::min({min_y, b.top(), b.bottom()});
        max_x        = std::max({max_x, b.left(), b.right()});
        max_y        = std::max({max_y, b.top(), b.bottom()});
    }

    return BoxF(min_x, min_y, max_x, max_y);
}

inline std::array<Vec2F, 8> SelectTool::selectionHandles(const std::vector<IDocument::TShape*>& shapes) const
{
    // Unit-square shapes keep their oriented frame.
    if (shapes.size() == 1 && usesUnitSquareFrame(*shapes.front()))
    {
        const Matrix3F& m = shapes.front()->transform();
        return {
            m.transformPoint({-0.5F, -0.5F}), // 0: Top-Left
            m.transformPoint({0.0F, -0.5F}),  // 1: Top-Middle
            m.transformPoint({+0.5F, -0.5F}), // 2: Top-Right
            m.transformPoint({+0.5F, 0.0F}),  // 3: Middle-Right
            m.transformPoint({+0.5F, +0.5F}), // 4: Bottom-Right
            m.transformPoint({0.0F, +0.5F}),  // 5: Bottom-Middle
            m.transformPoint({-0.5F, +0.5F}), // 6: Bottom-Left
            m.transformPoint({-0.5F, 0.0F}),  // 7: Middle-Left
        };
    }

    // Everything else uses the local axes-aligned bounds. During rotation the
    // frame is taken from the state captured on mouse-down so that it follows
    // the group instead of jumping to the changing AABB of every frame.
    const BoxF  box    = (mode_ == Mode::ROTATE) ? initial_group_box_ : groupBounds(shapes).value_or(BoxF{});
    const Vec2F center = (mode_ == Mode::ROTATE) ? group_center_ : box.center();
    const Vec2F half   = box.size() * 0.5F;
    const float angle  = (mode_ == Mode::ROTATE) ? group_visual_angle_ : 0.0F;
    const auto  rot    = Matrix3F::rotation(angle);

    return {
        center + rot.transformPoint({-half.x(), -half.y()}), center + rot.transformPoint({0.0F, -half.y()}),
        center + rot.transformPoint({+half.x(), -half.y()}), center + rot.transformPoint({+half.x(), 0.0F}),
        center + rot.transformPoint({+half.x(), +half.y()}), center + rot.transformPoint({0.0F, +half.y()}),
        center + rot.transformPoint({-half.x(), +half.y()}), center + rot.transformPoint({-half.x(), 0.0F}),
    };
}

inline Vec2F SelectTool::rotateHandlePos(const std::vector<IDocument::TShape*>& shapes) const
{
    if (shapes.size() == 1 && usesUnitSquareFrame(*shapes.front()))
    {
        const auto* shape = shapes.front();
        const float h     = std::abs(shape->scaling().y());
        if (h <= 1e-4F)
        {
            return shape->translation();
        }
        const float norm_offset_y = -0.5F - (ROTATE_HANDLE_DIST / h);
        return shape->transform().transformPoint({0.0F, norm_offset_y});
    }

    const BoxF  box    = (mode_ == Mode::ROTATE) ? initial_group_box_ : groupBounds(shapes).value_or(BoxF{});
    const Vec2F center = (mode_ == Mode::ROTATE) ? group_center_ : box.center();
    const Vec2F half   = box.size() * 0.5F;
    const float angle  = (mode_ == Mode::ROTATE) ? group_visual_angle_ : 0.0F;

    return center + Matrix3F::rotation(angle).transformPoint({0.0F, -half.y() - ROTATE_HANDLE_DIST});
}

inline bool SelectTool::hitTestPoint(const Vec2F& p, const Vec2F& target, float radius) const
{
    return std::abs(p.x() - target.x()) <= radius && std::abs(p.y() - target.y()) <= radius;
}

inline int SelectTool::hitTestResizeHandle(const std::vector<IDocument::TShape*>& shapes, const Vec2F& local_pos) const
{
    const auto handles = selectionHandles(shapes);
    for (int i = 0; i < 8; ++i)
    {
        if (hitTestPoint(local_pos, handles[i], toLocalRadius(HANDLE_HALF_SIZE + 3.0F, handles[i])))
        {
            return i;
        }
    }
    return -1;
}

inline bool SelectTool::processEvent(const IInputPump::Event& event)
{
    if (const auto* key = std::get_if<IInputPump::EventKeyboard>(&event))
    {
        if (key->type() == IInputPump::EventKeyboard::Type::KEY_DOWN)
        {
            return onKeyDown(*key);
        }
        return false;
    }

    const auto* mouse = std::get_if<IInputPump::EventMouse>(&event);
    if (mouse == nullptr)
    {
        return false;
    }

    using enum IInputPump::EventMouse::Type;
    switch (mouse->type())
    {
        case LEFT_BUTTON_PRESSING:
            return onMouseDown(mouse->cursorPos());

        case MOTION:
            return onMouseMove(mouse->cursorPos());

        case LEFT_BUTTON_RELEASING:
            return onMouseUp();

        default:
            return false;
    }
}

inline bool SelectTool::onKeyDown(const IInputPump::EventKeyboard& key)
{
    using Key = IInputPump::EventKeyboard::Key;

    if (key.key() == Key::ESC)
    {
        if (!selected_ids_.empty())
        {
            clearSelection();
            return true;
        }

        return false;
    }

    if (key.key() == Key::DEL || key.key() == Key::BACKSPACE)
    {
        if (!selected_ids_.empty())
        {
            for (const TObject::ID id : selected_ids_)
            {
                document_.removeShape(id);
            }

            clearSelection();
            return true;
        }
    }

    return false;
}

inline bool SelectTool::onMouseDown(const Vec2F& global_pos)
{
    const Vec2F local_pos = toLocal(global_pos);
    const auto  shapes    = selectedShapes();
    if (!shapes.empty())
    {
        // General rotation knob
        const Vec2F rotate_handle = rotateHandlePos(shapes);
        if (hitTestPoint(local_pos, rotate_handle, toLocalRadius(HANDLE_HALF_SIZE + 4.0F, rotate_handle)))
        {
            prev_mouse_local_   = local_pos;
            group_visual_angle_ = 0.0F;

            captureInitialStates(shapes);
            initial_group_box_ = groupBounds(shapes).value_or(BoxF{});
            group_center_      = initial_group_box_.center();

            mode_ = Mode::ROTATE;
            return true;
        }

        // 2. Any of the 8 resize handles
        const int handle_idx = hitTestResizeHandle(shapes, local_pos);
        if (handle_idx != -1)
        {
            const auto handles = selectionHandles(shapes);
            active_handle_idx_ = handle_idx;
            fixed_anchor_      = handles[(handle_idx + 4) % 8];

            captureInitialStates(shapes);
            initial_group_box_ = groupBounds(shapes).value_or(BoxF{});

            mode_ = Mode::RESIZE;
            return true;
        }
    }

    // 3. If a group (2+ shapes) is selected, clicking anywhere inside the bounding box moves the entire group.
    if (shapes.size() > 1)
    {
        if (const auto bounds = groupBounds(shapes); bounds.has_value())
        {
            const float min_x = std::min(bounds->left(), bounds->right());
            const float max_x = std::max(bounds->left(), bounds->right());
            const float min_y = std::min(bounds->top(), bounds->bottom());
            const float max_y = std::max(bounds->top(), bounds->bottom());

            if (local_pos.x() >= min_x && local_pos.x() <= max_x && local_pos.y() >= min_y && local_pos.y() <= max_y)
            {
                prev_mouse_local_ = local_pos;
                mode_             = Mode::MOVE;
                return true;
            }
        }
    }

    // 4. Checking for a hit on a shape under the cursor
    const std::optional<TObject::ID> hit = document_.hitTest(global_pos);
    if (hit.has_value())
    {
        if (!isSelected(*hit))
        {
            selected_ids_.clear();
            selected_ids_.insert(*hit);
        }

        prev_mouse_local_ = local_pos;
        mode_             = selected_ids_.empty() ? Mode::NONE : Mode::MOVE;
        return true;
    }

    // 5. Clicking on an empty space initiates selection via a resizable marquee (selection box).
    clearSelection();
    box_select_start_   = local_pos;
    box_select_current_ = local_pos;
    mode_               = Mode::BOX_SELECT;
    return true;
}

inline bool SelectTool::onMouseMove(const Vec2F& global_pos)
{
    if (mode_ == Mode::NONE)
    {
        return false;
    }

    const Vec2F local_pos = toLocal(global_pos);
    if (mode_ == Mode::BOX_SELECT)
    {
        box_select_current_ = local_pos;
        return true;
    }

    const auto shapes = selectedShapes();
    if (shapes.empty())
    {
        clearSelection();
        return false;
    }

    if (mode_ == Mode::MOVE)
    {
        const Vec2F delta = local_pos - prev_mouse_local_;
        for (auto* shape : shapes)
        {
            shape->translate(delta);
        }

        prev_mouse_local_ = local_pos;
        return true;
    }

    if (mode_ == Mode::ROTATE)
    {
        const Vec2F start_vec = prev_mouse_local_ - group_center_;
        const Vec2F curr_vec  = local_pos - group_center_;

        if (start_vec.len() > 1e-3F && curr_vec.len() > 1e-3F)
        {
            const float delta_angle = std::atan2(curr_vec.y(), curr_vec.x()) - std::atan2(start_vec.y(), start_vec.x());
            group_visual_angle_     = delta_angle;

            const auto rot = Matrix3F::rotation(delta_angle);
            for (auto* shape : shapes)
            {
                const auto it = initial_states_.find(shape->id());
                if (it == initial_states_.end())
                {
                    continue;
                }

                const InitialShapeState& st     = it->second;
                const Vec2F              rel    = st.translation - group_center_;
                const Vec2F              new_tr = group_center_ + rot.transformPoint(rel);

                shape->setTranslation(new_tr);
                shape->setRotation(st.rotation + delta_angle);
            }
        }

        return true;
    }

    if (mode_ == Mode::RESIZE)
    {
        const bool resize_x = (active_handle_idx_ % 2 == 0) || (active_handle_idx_ == 3) || (active_handle_idx_ == 7);
        const bool resize_y = (active_handle_idx_ % 2 == 0) || (active_handle_idx_ == 1) || (active_handle_idx_ == 5);

        // Determine the expected direction from `fixed_anchor_` to the active marker:
        // Right markers (2, 3, 4) -> +1; Left markers (0, 6, 7) -> -1
        const float dir_x =
            (active_handle_idx_ == 2 || active_handle_idx_ == 3 || active_handle_idx_ == 4) ? 1.0F : -1.0F;
        // Lower markers (4, 5, 6) -> +1; Upper markers (0, 1, 2) -> -1
        const float dir_y =
            (active_handle_idx_ == 4 || active_handle_idx_ == 5 || active_handle_idx_ == 6) ? 1.0F : -1.0F;

        // Single unit-square shape resize
        if (shapes.size() == 1 && usesUnitSquareFrame(*shapes.front()))
        {
            auto*       shape      = shapes.front();
            const auto& st         = initial_states_[shape->id()];
            const float angle      = st.rotation;
            const Vec2F diag_local = Matrix3F::rotation(-angle).transformPoint(local_pos - fixed_anchor_);

            float new_w    = st.scaling.x();
            float new_h    = st.scaling.y();
            float offset_x = 0.0F;
            float offset_y = 0.0F;

            if (resize_x)
            {
                new_w    = std::max(diag_local.x() * dir_x, MIN_SHAPE_SIZE);
                offset_x = dir_x * new_w * 0.5F;
            }

            if (resize_y)
            {
                new_h    = std::max(diag_local.y() * dir_y, MIN_SHAPE_SIZE);
                offset_y = dir_y * new_h * 0.5F;
            }

            const Vec2F new_center = fixed_anchor_ + Matrix3F::rotation(angle).transformPoint({offset_x, offset_y});
            shape->setScaling({new_w, new_h});
            shape->setTranslation(new_center);
            return true;
        }

        // Batch resize
        const float old_w = std::abs(initial_group_box_.size().x());
        const float old_h = std::abs(initial_group_box_.size().y());
        const Vec2F diag  = local_pos - fixed_anchor_;

        float sx = 1.0F;
        float sy = 1.0F;

        if (resize_x && old_w > 1e-3F)
        {
            const float new_w = std::max(diag.x() * dir_x, MIN_SHAPE_SIZE);
            sx                = new_w / old_w;
        }

        if (resize_y && old_h > 1e-3F)
        {
            const float new_h = std::max(diag.y() * dir_y, MIN_SHAPE_SIZE);
            sy                = new_h / old_h;
        }

        for (auto* shape : shapes)
        {
            const auto it = initial_states_.find(shape->id());
            if (it == initial_states_.end())
            {
                continue;
            }

            const InitialShapeState& st  = it->second;
            const Vec2F              rel = st.translation - fixed_anchor_;

            const float c     = std::cos(st.rotation);
            const float s     = std::sin(st.rotation);
            const float loc_x = std::hypot(sx * c, sy * s);
            const float loc_y = std::hypot(-sx * s, sy * c);

            /* MIN_SHAPE_SIZE is expressed in parent-space pixels, which is
               also the scaling unit of unit-square shapes. Polygons use
               scaling as a dimensionless multiplier, so for them only a tiny
               positive epsilon is appropriate; the group box itself is still
               clamped to MIN_SHAPE_SIZE above. */
            const float min_scaling = usesUnitSquareFrame(*shape) ? MIN_SHAPE_SIZE : 1e-4F;

            shape->setTranslation({fixed_anchor_.x() + rel.x() * sx, fixed_anchor_.y() + rel.y() * sy});
            shape->setScaling({
                std::max(st.scaling.x() * loc_x, min_scaling),
                std::max(st.scaling.y() * loc_y, min_scaling),
            });
        }

        return true;
    }

    return false;
}

inline bool SelectTool::onMouseUp()
{
    if (mode_ == Mode::NONE)
    {
        return false;
    }

    if (mode_ == Mode::BOX_SELECT)
    {
        const float min_x = std::min(box_select_start_.x(), box_select_current_.x());
        const float min_y = std::min(box_select_start_.y(), box_select_current_.y());
        const float max_x = std::max(box_select_start_.x(), box_select_current_.x());
        const float max_y = std::max(box_select_start_.y(), box_select_current_.y());

        if ((max_x - min_x) > MIN_MARQUEE_SIZE || (max_y - min_y) > MIN_MARQUEE_SIZE)
        {
            ShapeCollector collector;
            document_.accept(collector);

            for (const IDocument::TShape* shape : collector.shapes())
            {
                const BoxF  bounds = shape->axesAlignedBoundingBox();
                const float left   = std::min(bounds.left(), bounds.right());
                const float right  = std::max(bounds.left(), bounds.right());
                const float top    = std::min(bounds.top(), bounds.bottom());
                const float bottom = std::max(bounds.top(), bounds.bottom());

                const bool overlaps = (left <= max_x) && (right >= min_x) && (top <= max_y) && (bottom >= min_y);
                if (overlaps)
                {
                    selected_ids_.insert(shape->id());
                }
            }
        }
    }

    mode_               = Mode::NONE;
    active_handle_idx_  = -1;
    group_visual_angle_ = 0.0F;
    initial_states_.clear();

    return true;
}

inline void SelectTool::render()
{
    const IRenderer::Color accent = IRenderer::Color::BLUE;
    const IRenderer::Color fill   = IRenderer::Color::WHITE;

    // 1. Resizable selection frame
    if (mode_ == Mode::BOX_SELECT)
    {
        const Vec2F g1 = toGlobal(box_select_start_);
        const Vec2F g2 = toGlobal(box_select_current_);

        const Vec2F tl{std::min(g1.x(), g2.x()), std::min(g1.y(), g2.y())};
        const Vec2F tr{std::max(g1.x(), g2.x()), std::min(g1.y(), g2.y())};
        const Vec2F br{std::max(g1.x(), g2.x()), std::max(g1.y(), g2.y())};
        const Vec2F bl{std::min(g1.x(), g2.x()), std::max(g1.y(), g2.y())};

        renderer_.renderLine(accent, tl, tr);
        renderer_.renderLine(accent, tr, br);
        renderer_.renderLine(accent, br, bl);
        renderer_.renderLine(accent, bl, tl);
    }

    const auto shapes = selectedShapes();
    if (shapes.empty())
    {
        return;
    }

    // 2. Overall selection frame, 8 handles, and a rotation handle
    const auto  handles = selectionHandles(shapes);
    const Vec2F top_mid = toGlobal(handles[1]);
    const Vec2F rot_pt  = toGlobal(rotateHandlePos(shapes));

    for (int i = 0; i < 4; ++i)
    {
        const Vec2F c1 = toGlobal(handles[i * 2]);
        const Vec2F c2 = toGlobal(handles[((i + 1) % 4) * 2]);
        renderer_.renderLine(accent, c1, c2);
    }

    renderer_.renderLine(accent, top_mid, rot_pt);

    const BoxF rot_box(rot_pt.x() - HANDLE_HALF_SIZE, rot_pt.y() - HANDLE_HALF_SIZE, rot_pt.x() + HANDLE_HALF_SIZE,
                       rot_pt.y() + HANDLE_HALF_SIZE);
    renderer_.renderEllipse(fill, rot_box);
    renderer_.renderEllipseLines(accent, rot_box);

    for (const Vec2F& h : handles)
    {
        const Vec2F g = toGlobal(h);
        const BoxF  h_box(g.x() - HANDLE_HALF_SIZE, g.y() - HANDLE_HALF_SIZE, g.x() + HANDLE_HALF_SIZE,
                          g.y() + HANDLE_HALF_SIZE);
        renderer_.renderRect(fill, h_box);
        renderer_.renderRectLines(accent, h_box);
    }
}

}; // namespace Tools

}; // namespace Murugov
