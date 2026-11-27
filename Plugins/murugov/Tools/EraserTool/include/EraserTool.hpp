#pragma once

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/IToolManager.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/TObject.hpp"
#include "Utils/Vec2F.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <unordered_set>
#include <variant>
#include <vector>

namespace Murugov
{

namespace Tools
{

constexpr IToolManager::ATool::ID ERASER_TOOL_ID = 8;

/* ====================== EraserTool ====================== */

class EraserTool final : public IToolManager::ATool
{
public:
    EraserTool(ID tool_id, IDocument& document, IRenderer& renderer);
    ~EraserTool() override = default;

    EraserTool(const EraserTool&)            = delete;
    EraserTool(EraserTool&&)                 = delete;
    EraserTool& operator=(const EraserTool&) = delete;
    EraserTool& operator=(EraserTool&&)      = delete;

    ID   id() const override { return id_; }
    bool processEvent(const IInputPump::Event& event) override;
    void render();
    void deactivate();

private:
    static constexpr float DEFAULT_RADIUS = 14.0F;
    static constexpr float MIN_RADIUS     = 4.0F;
    static constexpr float MAX_RADIUS     = 80.0F;
    static constexpr float RADIUS_STEP    = 3.0F;

    void eraseAt(const Vec2F& global_pos);
    void eraseAlongSegment(const Vec2F& from_global, const Vec2F& to_global);

    ID         id_;
    IDocument& document_;
    IRenderer& renderer_;

    float radius_     = DEFAULT_RADIUS;
    bool  is_erasing_ = false;
    bool  has_cursor_ = false;
    Vec2F cursor_global_;
    Vec2F prev_erase_global_;
};

/* ====================== Implementation ====================== */

inline EraserTool::EraserTool(ID tool_id, IDocument& document, IRenderer& renderer) :
    id_(tool_id), document_(document), renderer_(renderer)
{
}

inline void EraserTool::deactivate()
{
    has_cursor_ = false;
    is_erasing_ = false;
}

inline void EraserTool::eraseAt(const Vec2F& global_pos)
{
    std::unordered_set<TObject::ID> to_remove;

    if (const auto hit = document_.hitTest(global_pos); hit.has_value())
    {
        to_remove.insert(*hit);
    }

    constexpr int                   SAMPLE_DIRS = 12;
    constexpr std::array<float, 2>  RINGS       = {0.5F, 1.0F};

    for (float ring_scale : RINGS)
    {
        const float r = radius_ * ring_scale;
        for (int i = 0; i < SAMPLE_DIRS; ++i)
        {
            const float angle = (2.0F * std::numbers::pi_v<float> * static_cast<float>(i)) /
                                static_cast<float>(SAMPLE_DIRS);
            const Vec2F sample{global_pos.x() + std::cos(angle) * r, global_pos.y() + std::sin(angle) * r};

            if (const auto hit = document_.hitTest(sample); hit.has_value())
            {
                to_remove.insert(*hit);
            }
        }
    }

    for (const TObject::ID shape_id : to_remove)
    {
        document_.removeShape(shape_id);
    }
}

inline void EraserTool::eraseAlongSegment(const Vec2F& from_global, const Vec2F& to_global)
{
    const Vec2F diff = to_global - from_global;
    const float dist = diff.len();
    const float step = std::max(radius_ * 0.4F, 2.0F);

    if (dist <= step)
    {
        eraseAt(to_global);
        return;
    }

    const int steps = static_cast<int>(std::ceil(dist / step));
    for (int i = 1; i <= steps; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(steps);
        eraseAt(from_global + diff * t);
    }
}

inline bool EraserTool::processEvent(const IInputPump::Event& event)
{
    const auto* mouse = std::get_if<IInputPump::EventMouse>(&event);
    if (mouse == nullptr)
    {
        return false;
    }

    cursor_global_ = mouse->cursorPos();
    has_cursor_    = true;

    using enum IInputPump::EventMouse::Type;
    switch (mouse->type())
    {
        case LEFT_BUTTON_PRESSING:
        {
            is_erasing_        = true;
            prev_erase_global_ = cursor_global_;
            eraseAt(cursor_global_);
            return true;
        }

        case MOTION:
        {
            if (is_erasing_)
            {
                eraseAlongSegment(prev_erase_global_, cursor_global_);
                prev_erase_global_ = cursor_global_;
                return true;
            }
            return false;
        }

        case LEFT_BUTTON_RELEASING:
        {
            if (is_erasing_)
            {
                is_erasing_ = false;
                return true;
            }
            return false;
        }

        case WHEEL_FORWARD:
        {
            radius_ = std::min(radius_ + RADIUS_STEP, MAX_RADIUS);
            return true;
        }

        case WHEEL_BACKWARD:
        {
            radius_ = std::max(radius_ - RADIUS_STEP, MIN_RADIUS);
            return true;
        }

        default:
            return false;
    }
}

inline void EraserTool::render()
{
    if (!has_cursor_)
    {
        return;
    }

    const BoxF ring_box(cursor_global_.x() - radius_, cursor_global_.y() - radius_,
                        cursor_global_.x() + radius_, cursor_global_.y() + radius_);

    const IRenderer::Color ring_color = is_erasing_ ? IRenderer::Color::RED : IRenderer::Color::WHITE;
    renderer_.renderEllipseLines(ring_color, ring_box);
}

} // namespace Tools

} // namespace Murugov