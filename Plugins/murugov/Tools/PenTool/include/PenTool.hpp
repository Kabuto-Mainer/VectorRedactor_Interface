#pragma once

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/IToolManager.hpp"
#include "../../Palette/IMurugovPalette.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/TObject.hpp"
#include "Utils/Vec2F.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace Murugov
{

namespace Tools
{

constexpr IToolManager::ATool::ID PEN_TOOL_ID = 6;

class PenColorComponent : public IDocument::AShapeComponent
{
public:
    constexpr static const ID COID = 4;

    IRenderer::Color color() const { return color_; }
    void             setColor(const IRenderer::Color& color) { color_ = color; }

protected:
    IRenderer::Color color_ = IRenderer::Color::WHITE;
};

/* ====================== PenStrokeShape ====================== */

class PenStrokeShape final : public IDocument::TShape, public PenColorComponent
{
public:
    static constexpr IDocument::TShape::TypeID PEN_STROKE_SHAPE_ID = 102;
    static constexpr float                     HIT_TOLERANCE       = 6.0F;

    explicit PenStrokeShape(const std::vector<Vec2F>& world_points,
                            IRenderer::Color          stroke_color = IRenderer::Color::WHITE);
    ~PenStrokeShape() override = default;

    bool processEvent(const IInputPump::Event& event) override;
    void render() override;

    bool   hitTest(Vec2F const& pos) const override;
    BoxF   axesAlignedBoundingBox() const override;
    TypeID tid() const override { return PEN_STROKE_SHAPE_ID; }

    Error queryComponent(IDocument::AShapeComponent::ID coid, IDocument::AShapeComponent** component) override;
    Error queryComponent(IDocument::AShapeComponent::ID     coid,
                         IDocument::AShapeComponent const** component) const override;

private:
    std::vector<Vec2F> local_points_;
};

/* ====================== PenTool ====================== */

class PenTool final : public IToolManager::ATool
{
public:
    PenTool(ID tool_id, IDocument& document, IRenderer& renderer);
    ~PenTool() override = default;

    PenTool(const PenTool&)            = delete;
    PenTool(PenTool&&)                 = delete;
    PenTool& operator=(const PenTool&) = delete;
    PenTool& operator=(PenTool&&)      = delete;

    ID   id() const override { return id_; }
    bool processEvent(const IInputPump::Event& event) override;
    void render();

private:
    Vec2F            toDocLocal(const Vec2F& global_pos) const;
    Vec2F            toDocGlobal(const Vec2F& local_pos) const;
    IRenderer::Color currentColor() const;

    static constexpr float MIN_POINT_DIST = 3.0F;

    ID                 id_;
    IDocument&         document_;
    IRenderer&         renderer_;
    bool               is_drawing_ = false;
    std::vector<Vec2F> current_points_;
};

/* ====================== Implementation ====================== */

inline PenStrokeShape::PenStrokeShape(const std::vector<Vec2F>& doc_points, IRenderer::Color stroke_color)
{
    setColor(stroke_color);
    if (doc_points.empty())
    {
        return;
    }

    float min_x = doc_points.front().x();
    float max_x = min_x;
    float min_y = doc_points.front().y();
    float max_y = min_y;

    for (const Vec2F& p : doc_points)
    {
        min_x = std::min(min_x, p.x());
        max_x = std::max(max_x, p.x());
        min_y = std::min(min_y, p.y());
        max_y = std::max(max_y, p.y());
    }

    const float w = std::max(max_x - min_x, 4.0F);
    const float h = std::max(max_y - min_y, 4.0F);
    const Vec2F center{(min_x + max_x) * 0.5F, (min_y + max_y) * 0.5F};

    setTranslation(center);
    setScaling({w, h});

    local_points_.reserve(doc_points.size());
    for (const Vec2F& p : doc_points)
    {
        local_points_.push_back({(p.x() - center.x()) / w, (p.y() - center.y()) / h});
    }
}

inline bool PenStrokeShape::processEvent(const IInputPump::Event& /*event*/)
{
    return false;
}

inline void PenStrokeShape::render()
{
    if (local_points_.empty())
    {
        return;
    }

    IRenderer* renderer = nullptr;
    if (ApplicationCore::instance().queryInterface(IID_RENDERER, reinterpret_cast<void**>(&renderer)) !=
            ApplicationCore::Error::SUCCESS ||
        renderer == nullptr)
    {
        return;
    }

    if (local_points_.size() == 1)
    {
        const Vec2F p = mapLocalToGlobal(local_points_.front());
        renderer->renderEllipse(color(), BoxF{p.x() - 2.0F, p.y() - 2.0F, p.x() + 2.0F, p.y() + 2.0F});
        return;
    }

    for (std::size_t i = 1; i < local_points_.size(); ++i)
    {
        const Vec2F p1 = mapLocalToGlobal(local_points_[i - 1]);
        const Vec2F p2 = mapLocalToGlobal(local_points_[i]);
        renderer->renderLine(color(), p1, p2);
    }
}

inline bool PenStrokeShape::hitTest(Vec2F const& pos) const
{
    if (local_points_.empty())
    {
        return false;
    }

    if (local_points_.size() == 1)
    {
        const Vec2F p = mapLocalToGlobal(local_points_.front());
        return (pos - p).len() <= HIT_TOLERANCE;
    }

    for (std::size_t i = 1; i < local_points_.size(); ++i)
    {
        const Vec2F a  = mapLocalToGlobal(local_points_[i - 1]);
        const Vec2F b  = mapLocalToGlobal(local_points_[i]);
        const Vec2F ab = b - a;
        const float l2 = ab.len2();

        float t = 0.0F;
        if (l2 > 1e-5F)
        {
            const Vec2F ap = pos - a;
            t              = std::clamp((ap.x() * ab.x() + ap.y() * ab.y()) / l2, 0.0F, 1.0F);
        }

        const Vec2F closest = a + ab * t;
        if ((pos - closest).len() <= HIT_TOLERANCE)
        {
            return true;
        }
    }

    return false;
}

inline BoxF PenStrokeShape::axesAlignedBoundingBox() const
{
    if (local_points_.empty())
    {
        return BoxF{translation(), translation()};
    }

    Vec2F first = transform().transformPoint(local_points_.front());
    float min_x = first.x();
    float max_x = first.x();
    float min_y = first.y();
    float max_y = first.y();

    for (std::size_t i = 1; i < local_points_.size(); ++i)
    {
        const Vec2F p = transform().transformPoint(local_points_[i]);
        min_x         = std::min(min_x, p.x());
        max_x         = std::max(max_x, p.x());
        min_y         = std::min(min_y, p.y());
        max_y         = std::max(max_y, p.y());
    }

    return BoxF{min_x, min_y, max_x, max_y};
}

inline IDocument::TShape::Error PenStrokeShape::queryComponent(IDocument::AShapeComponent::ID coid,
                                                               IDocument::AShapeComponent**   component)
{
    if (component == nullptr)
    {
        return Error::INVALID_ARGS;
    }

    if (coid == PenColorComponent::COID)
    {
        *component = static_cast<PenColorComponent*>(this);
        return Error::SUCCESS;
    }

    *component = nullptr;
    return Error::COMPONENT_NOT_FOUND;
}

inline IDocument::TShape::Error PenStrokeShape::queryComponent(IDocument::AShapeComponent::ID     coid,
                                                               IDocument::AShapeComponent const** component) const
{
    if (component == nullptr)
    {
        return Error::INVALID_ARGS;
    }

    if (coid == PenColorComponent::COID)
    {
        *component = static_cast<const PenColorComponent*>(this);
        return Error::SUCCESS;
    }

    *component = nullptr;
    return Error::COMPONENT_NOT_FOUND;
}

inline PenTool::PenTool(ID tool_id, IDocument& document, IRenderer& renderer) :
    id_(tool_id), document_(document), renderer_(renderer)
{
}

inline Vec2F PenTool::toDocLocal(const Vec2F& global_pos) const
{
    if (const auto* node = dynamic_cast<const TObject*>(&document_))
    {
        return node->mapGlobalToLocal(global_pos);
    }
    return global_pos;
}

inline Vec2F PenTool::toDocGlobal(const Vec2F& local_pos) const
{
    if (const auto* node = dynamic_cast<const TObject*>(&document_))
    {
        return node->mapLocalToGlobal(local_pos);
    }
    return local_pos;
}

inline IRenderer::Color PenTool::currentColor() const
{
    IPalette* palette = nullptr;
    if (ApplicationCore::instance().queryInterface(Murugov::IID_PALETTE, reinterpret_cast<void**>(&palette)) ==
            ApplicationCore::Error::SUCCESS &&
        palette != nullptr)
    {
        return palette->selectedColor();
    }
    return IRenderer::Color::WHITE;
}

inline bool PenTool::processEvent(const IInputPump::Event& event)
{
    const auto* mouse = std::get_if<IInputPump::EventMouse>(&event);
    if (mouse == nullptr)
    {
        return false;
    }

    using enum IInputPump::EventMouse::Type;
    switch (mouse->type())
    {
        case LEFT_BUTTON_PRESSING:
        {
            is_drawing_ = true;
            current_points_.clear();
            current_points_.push_back(toDocLocal(mouse->cursorPos()));
            return true;
        }

        case MOTION:
        {
            if (!is_drawing_)
            {
                return false;
            }
            const Vec2F pt = toDocLocal(mouse->cursorPos());
            if (current_points_.empty() || (pt - current_points_.back()).len() >= MIN_POINT_DIST)
            {
                current_points_.push_back(pt);
            }
            return true;
        }

        case LEFT_BUTTON_RELEASING:
        {
            if (!is_drawing_)
            {
                return false;
            }
            is_drawing_ = false;
            if (!current_points_.empty())
            {
                auto shape = std::make_unique<PenStrokeShape>(current_points_, currentColor());
                document_.addShape(std::move(shape));
                current_points_.clear();
            }
            return true;
        }

        default:
            return false;
    }
}

inline void PenTool::render()
{
    if (!is_drawing_ || current_points_.empty())
    {
        return;
    }

    if (current_points_.size() == 1)
    {
        const Vec2F p = toDocGlobal(current_points_.front());
        renderer_.renderEllipse(currentColor(), BoxF{p.x() - 2.0F, p.y() - 2.0F, p.x() + 2.0F, p.y() + 2.0F});
        return;
    }

    for (std::size_t i = 1; i < current_points_.size(); ++i)
    {
        renderer_.renderLine(currentColor(), toDocGlobal(current_points_[i - 1]),
            toDocGlobal(current_points_[i]));
    }
}

} // namespace Tools

} // namespace Murugov