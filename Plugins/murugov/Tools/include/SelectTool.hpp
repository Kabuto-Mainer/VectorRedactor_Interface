#pragma once

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
    SelectTool(ID id, IDocument& document, IRenderer& renderer, const BoxF& ui_box);
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

    struct InitialShapeState
    {
        Vec2F translation;
        Vec2F scaling;
        float rotation = 0.0F;
    };

    static constexpr float HANDLE_HALF_SIZE   = 4.5F;
    static constexpr float ROTATE_HANDLE_DIST = 26.0F;
    static constexpr float MIN_SHAPE_SIZE     = 4.0F;

    Vec2F toLocal(const Vec2F& global_pos) const;
    Vec2F toGlobal(const Vec2F& local_pos) const;

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
    BoxF       ui_box_;

    std::unordered_set<TObject::ID> selected_ids_;
    Mode                            mode_       = Mode::NONE;

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

/* ====================== Implementation ====================== */

inline SelectTool::SelectTool(ID id, IDocument& document, IRenderer& renderer, const BoxF& ui_box) :
    id_(id), document_(document), renderer_(renderer), ui_box_(ui_box)
{
}

inline IToolManager::ATool::ID SelectTool::id() const
{
    return id_;
}

inline Vec2F SelectTool::toLocal(const Vec2F& global_pos) const
{
    if (const auto* node = dynamic_cast<const TObject*>(&document_))
    {
        return node->mapGlobalToLocal(global_pos);
    }
    return global_pos;
}

inline Vec2F SelectTool::toGlobal(const Vec2F& local_pos) const
{
    if (const auto* node = dynamic_cast<const TObject*>(&document_))
    {
        return node->mapLocalToGlobal(local_pos);
    }
    return local_pos;
}

inline std::vector<IDocument::TShape*> SelectTool::selectedShapes() const
{
    std::vector<IDocument::TShape*> result;
    if (selected_ids_.empty())
    {
        return result;
    }

    if (const auto* node = dynamic_cast<const TObject*>(&document_))
    {
        result.reserve(selected_ids_.size());
        for (const auto& child : node->children())
        {
            if (child && selected_ids_.contains(child->id()))
            {
                if (auto* shape = dynamic_cast<IDocument::TShape*>(child.get()))
                {
                    result.push_back(shape);
                }
            }
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
    // For a single shape, we use its own oriented bounding box.
    if (shapes.size() == 1)
    {
        const Matrix3F& m = shapes.front()->transform();
        return {
            m.transformPoint({-0.5F, -0.5F}), // 0: Top-Left
            m.transformPoint({ 0.0F, -0.5F}), // 1: Top-Middle
            m.transformPoint({+0.5F, -0.5F}), // 2: Top-Right
            m.transformPoint({+0.5F,  0.0F}), // 3: Middle-Right
            m.transformPoint({+0.5F, +0.5F}), // 4: Bottom-Right
            m.transformPoint({ 0.0F, +0.5F}), // 5: Bottom-Middle
            m.transformPoint({-0.5F, +0.5F}), // 6: Bottom-Left
            m.transformPoint({-0.5F,  0.0F}), // 7: Middle-Left
        };
    }

    // For a group of several shapes, we construct a common axial frame
    const BoxF  box    = (mode_ == Mode::ROTATE) ? initial_group_box_ : groupBounds(shapes).value_or(BoxF{});
    const Vec2F center = box.center();
    const Vec2F half   = box.size() * 0.5F;
    const float angle  = (mode_ == Mode::ROTATE) ? group_visual_angle_ : 0.0F;
    const auto  rot    = Matrix3F::rotation(angle);

    return {
        center + rot.transformPoint({-half.x(), -half.y()}),
        center + rot.transformPoint({     0.0F, -half.y()}),
        center + rot.transformPoint({+half.x(), -half.y()}),
        center + rot.transformPoint({+half.x(),      0.0F}),
        center + rot.transformPoint({+half.x(), +half.y()}),
        center + rot.transformPoint({     0.0F, +half.y()}),
        center + rot.transformPoint({-half.x(), +half.y()}),
        center + rot.transformPoint({-half.x(),      0.0F}),
    };
}

inline Vec2F SelectTool::rotateHandlePos(const std::vector<IDocument::TShape*>& shapes) const
{
    if (shapes.size() == 1)
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
    const Vec2F center = box.center();
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
        if (hitTestPoint(local_pos, handles[i], HANDLE_HALF_SIZE + 3.0F))
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
    if (ui_box_.contains(global_pos))
    {
        return false;
    }

    const Vec2F local_pos = toLocal(global_pos);
    const auto  shapes    = selectedShapes();
    if (!shapes.empty())
    {
        // General rotation knob
        if (hitTestPoint(local_pos, rotateHandlePos(shapes), HANDLE_HALF_SIZE + 4.0F))
        {
            prev_mouse_local_   = local_pos;
            group_visual_angle_ = 0.0F;

            captureInitialStates(shapes);
            if (shapes.size() == 1)
            {
                group_center_ = shapes.front()->translation();
            }
            else
            {
                initial_group_box_ = groupBounds(shapes).value_or(BoxF{});
                group_center_      = initial_group_box_.center();
            }

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
            if (shapes.size() > 1)
            {
                initial_group_box_ = groupBounds(shapes).value_or(BoxF{});
            }

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

            if (local_pos.x() >= min_x && local_pos.x() <= max_x &&
                local_pos.y() >= min_y && local_pos.y() <= max_y)
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
            const float delta_angle =
                std::atan2(curr_vec.y(), curr_vec.x()) - std::atan2(start_vec.y(), start_vec.x());
            group_visual_angle_ = delta_angle;

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
        const float dir_x = (active_handle_idx_ == 2 || active_handle_idx_ == 3 || active_handle_idx_ == 4) ? 1.0F : -1.0F;
        // Lower markers (4, 5, 6) -> +1; Upper markers (0, 1, 2) -> -1
        const float dir_y = (active_handle_idx_ == 4 || active_handle_idx_ == 5 || active_handle_idx_ == 6) ? 1.0F : -1.0F;

        // Single-item resize
        if (shapes.size() == 1)
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

            shape->setTranslation({fixed_anchor_.x() + rel.x() * sx, fixed_anchor_.y() + rel.y() * sy});
            shape->setScaling({
                std::max(st.scaling.x() * loc_x, MIN_SHAPE_SIZE),
                std::max(st.scaling.y() * loc_y, MIN_SHAPE_SIZE),
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

        if ((max_x - min_x) > 2.0F || (max_y - min_y) > 2.0F)
        {
            if (const auto* doc_node = dynamic_cast<const TObject*>(&document_))
            {
                for (const auto& child : doc_node->children())
                {
                    const auto* shape = dynamic_cast<const IDocument::TShape*>(child.get());
                    if (shape == nullptr)
                    {
                        continue;
                    }

                    const BoxF  b  = shape->axesAlignedBoundingBox();
                    const float bl = std::min(b.left(), b.right());
                    const float br = std::max(b.left(), b.right());
                    const float bt = std::min(b.top(), b.bottom());
                    const float bb = std::max(b.top(), b.bottom());

                    const bool overlaps = (bl <= max_x) && (br >= min_x) && (bt <= max_y) && (bb >= min_y);
                    if (overlaps)
                    {
                        selected_ids_.insert(shape->id());
                    }
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
    const IRenderer::Color accent = IRenderer::Color::Blue;
    const IRenderer::Color fill   = IRenderer::Color::White;

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

    const BoxF rot_box(rot_pt.x() - HANDLE_HALF_SIZE, rot_pt.y() - HANDLE_HALF_SIZE,
                       rot_pt.x() + HANDLE_HALF_SIZE, rot_pt.y() + HANDLE_HALF_SIZE);
    renderer_.renderEllipse(fill, rot_box);
    renderer_.renderEllipseLines(accent, rot_box);

    for (const Vec2F& h : handles)
    {
        const Vec2F g = toGlobal(h);
        const BoxF  h_box(g.x() - HANDLE_HALF_SIZE, g.y() - HANDLE_HALF_SIZE,
                          g.x() + HANDLE_HALF_SIZE, g.y() + HANDLE_HALF_SIZE);
        renderer_.renderRect(fill, h_box);
        renderer_.renderRectLines(accent, h_box);
    }
}

}; // namespace Tools

}; // namespace Murugov