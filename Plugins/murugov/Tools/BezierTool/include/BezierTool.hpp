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
#include <cmath>
#include <vector>

namespace Murugov
{

namespace Tools
{

constexpr IToolManager::ATool::ID BEZIER_TOOL_ID = 7;

class BezierColorComponent : public IDocument::AShapeComponent
{
public:
    constexpr static const ID COID = 4;

    IRenderer::Color color() const { return color_; }
    void             setColor(const IRenderer::Color& color) { color_ = color; }

protected:
    IRenderer::Color color_ = IRenderer::Color::WHITE;
};

struct BezierNode
{
    Vec2F anchor;
    Vec2F handle_in;  // Absolute (in document coordinates) position of the incoming tangent
    Vec2F handle_out; // Absolute position of outgoing tangent
};

inline Vec2F evalCubicBezier(const Vec2F& p0, const Vec2F& p1, const Vec2F& p2, const Vec2F& p3, float t)
{
    const float u   = 1.0F - t;
    const float tt  = t * t;
    const float uu  = u * u;
    const float uuu = uu * u;
    const float ttt = tt * t;

    return p0 * uuu + p1 * (3.0F * uu * t) + p2 * (3.0F * u * tt) + p3 * ttt;
}

inline std::vector<Vec2F> flattenBezierSpline(const std::vector<BezierNode>& nodes, bool closed,
                                              int steps_per_segment = 24)
{
    std::vector<Vec2F> poly;
    if (nodes.empty())
    {
        return poly;
    }

    if (nodes.size() == 1)
    {
        poly.push_back(nodes.front().anchor);
        return poly;
    }

    const std::size_t seg_count = closed ? nodes.size() : (nodes.size() - 1);
    poly.reserve(seg_count * static_cast<std::size_t>(steps_per_segment) + 1);
    poly.push_back(nodes.front().anchor);

    for (std::size_t i = 0; i < seg_count; ++i)
    {
        const BezierNode& a = nodes[i];
        const BezierNode& b = nodes[(i + 1) % nodes.size()];

        for (int s = 1; s <= steps_per_segment; ++s)
        {
            const float t = static_cast<float>(s) / static_cast<float>(steps_per_segment);
            poly.push_back(evalCubicBezier(a.anchor, a.handle_out, b.handle_in, b.anchor, t));
        }
    }

    return poly;
}

class BezierCurveShape final : public IDocument::TShape, public BezierColorComponent
{
public:
    static constexpr IDocument::TShape::TypeID BEZIER_SHAPE_ID = 103;
    static constexpr float                     HIT_TOLERANCE   = 6.0F;

    BezierCurveShape(const std::vector<BezierNode>& doc_nodes, bool closed,
                     IRenderer::Color stroke_color = IRenderer::Color::WHITE);
    ~BezierCurveShape() override = default;

    bool processEvent(const IInputPump::Event& event) override;
    void render() override;

    bool   hitTest(Vec2F const& pos) const override;
    BoxF   axesAlignedBoundingBox() const override;
    TypeID tid() const override { return BEZIER_SHAPE_ID; }

    Error queryComponent(IDocument::AShapeComponent::ID coid, IDocument::AShapeComponent** component) override;
    Error queryComponent(IDocument::AShapeComponent::ID     coid,
                         IDocument::AShapeComponent const** component) const override;

private:
    std::vector<Vec2F> local_polyline_;
};

/* ====================== BezierTool ====================== */

class BezierTool final : public IToolManager::ATool
{
public:
    BezierTool(ID tool_id, IDocument& document, IRenderer& renderer);
    ~BezierTool() override = default;

    BezierTool(const BezierTool&)            = delete;
    BezierTool(BezierTool&&)                 = delete;
    BezierTool& operator=(const BezierTool&) = delete;
    BezierTool& operator=(BezierTool&&)      = delete;

    ID   id() const override { return id_; }
    bool processEvent(const IInputPump::Event& event) override;
    void render();

private:
    static constexpr float CLOSE_SNAP_RADIUS = 8.0F;
    static constexpr float NODE_HALF_SIZE    = 4.0F;

    Vec2F toDocLocal(const Vec2F& global_pos) const;
    Vec2F toDocGlobal(const Vec2F& local_pos) const;

    void commitCurve(bool closed);
    void cancelCurve();

    ID                      id_;
    IDocument&              document_;
    IRenderer&              renderer_;
    std::vector<BezierNode> nodes_;
    Vec2F                   cursor_local_;
    bool                    has_cursor_  = false;
    bool                    is_dragging_ = false;
};

/* ====================== Implementation ====================== */

inline BezierCurveShape::BezierCurveShape(const std::vector<BezierNode>& doc_nodes, bool closed,
                                          IRenderer::Color stroke_color)
{
    setColor(stroke_color);
    const std::vector<Vec2F> pts = flattenBezierSpline(doc_nodes, closed);
    if (pts.empty())
    {
        return;
    }

    float min_x = pts.front().x();
    float max_x = min_x;
    float min_y = pts.front().y();
    float max_y = min_y;

    for (const Vec2F& p : pts)
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

    local_polyline_.reserve(pts.size());
    for (const Vec2F& p : pts)
    {
        local_polyline_.push_back({(p.x() - center.x()) / w, (p.y() - center.y()) / h});
    }
}

inline bool BezierCurveShape::processEvent(const IInputPump::Event& /*event*/)
{
    return false;
}

inline void BezierCurveShape::render()
{
    if (local_polyline_.empty())
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

    if (local_polyline_.size() == 1)
    {
        const Vec2F p = mapLocalToGlobal(local_polyline_.front());
        renderer->renderEllipse(color(), BoxF{p.x() - 2.0F, p.y() - 2.0F, p.x() + 2.0F, p.y() + 2.0F});
        return;
    }

    for (std::size_t i = 1; i < local_polyline_.size(); ++i)
    {
        renderer->renderLine(color(), mapLocalToGlobal(local_polyline_[i - 1]),
                             mapLocalToGlobal(local_polyline_[i]));
    }
}

inline bool BezierCurveShape::hitTest(Vec2F const& pos) const
{
    if (local_polyline_.empty())
    {
        return false;
    }

    if (local_polyline_.size() == 1)
    {
        return (pos - mapLocalToGlobal(local_polyline_.front())).len() <= HIT_TOLERANCE;
    }

    for (std::size_t i = 1; i < local_polyline_.size(); ++i)
    {
        const Vec2F a  = mapLocalToGlobal(local_polyline_[i - 1]);
        const Vec2F b  = mapLocalToGlobal(local_polyline_[i]);
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

inline BoxF BezierCurveShape::axesAlignedBoundingBox() const
{
    if (local_polyline_.empty())
    {
        return BoxF{translation(), translation()};
    }

    Vec2F first = transform().transformPoint(local_polyline_.front());
    float min_x = first.x();
    float max_x = first.x();
    float min_y = first.y();
    float max_y = first.y();

    for (std::size_t i = 1; i < local_polyline_.size(); ++i)
    {
        const Vec2F p = transform().transformPoint(local_polyline_[i]);
        min_x         = std::min(min_x, p.x());
        max_x         = std::max(max_x, p.x());
        min_y         = std::min(min_y, p.y());
        max_y         = std::max(max_y, p.y());
    }

    return BoxF{min_x, min_y, max_x, max_y};
}

inline IDocument::TShape::Error BezierCurveShape::queryComponent(IDocument::AShapeComponent::ID coid,
                                                                 IDocument::AShapeComponent**   component)
{
    if (component == nullptr)
    {
        return Error::INVALID_ARGS;
    }

    if (coid == BezierColorComponent::COID)
    {
        *component = static_cast<BezierColorComponent*>(this);
        return Error::SUCCESS;
    }

    *component = nullptr;
    return Error::COMPONENT_NOT_FOUND;
}

inline IDocument::TShape::Error BezierCurveShape::queryComponent(IDocument::AShapeComponent::ID     coid,
                                                                 IDocument::AShapeComponent const** component) const
{
    if (component == nullptr)
    {
        return Error::INVALID_ARGS;
    }

    if (coid == BezierColorComponent::COID)
    {
        *component = static_cast<const BezierColorComponent*>(this);
        return Error::SUCCESS;
    }

    *component = nullptr;
    return Error::COMPONENT_NOT_FOUND;
}

inline BezierTool::BezierTool(ID tool_id, IDocument& document, IRenderer& renderer) :
    id_(tool_id), document_(document), renderer_(renderer)
{
}

inline Vec2F BezierTool::toDocLocal(const Vec2F& global_pos) const
{
    if (const auto* node = dynamic_cast<const TObject*>(&document_))
    {
        return node->mapGlobalToLocal(global_pos);
    }
    return global_pos;
}

inline Vec2F BezierTool::toDocGlobal(const Vec2F& local_pos) const
{
    if (const auto* node = dynamic_cast<const TObject*>(&document_))
    {
        return node->mapLocalToGlobal(local_pos);
    }
    return local_pos;
}

inline void BezierTool::commitCurve(bool closed)
{
    if (nodes_.size() >= 2)
    {
        auto shape = std::make_unique<BezierCurveShape>(nodes_, closed, IRenderer::Color::WHITE);
        document_.addShape(std::move(shape));
    }
    cancelCurve();
}

inline void BezierTool::cancelCurve()
{
    nodes_.clear();
    is_dragging_ = false;
}

inline bool BezierTool::processEvent(const IInputPump::Event& event)
{
    if (const auto* key = std::get_if<IInputPump::EventKeyboard>(&event))
    {
        if (key->type() == IInputPump::EventKeyboard::Type::KEY_DOWN &&
            key->key() == IInputPump::EventKeyboard::Key::ESC && !nodes_.empty())
        {
            cancelCurve();
            return true;
        }
        return false;
    }

    const auto* mouse = std::get_if<IInputPump::EventMouse>(&event);
    if (mouse == nullptr)
    {
        return false;
    }

    const Vec2F pos = toDocLocal(mouse->cursorPos());
    cursor_local_   = pos;
    has_cursor_     = true;

    using enum IInputPump::EventMouse::Type;
    switch (mouse->type())
    {
        case LEFT_BUTTON_PRESSING:
        {
            // Клик по первому узлу замыкает кривую
            if (nodes_.size() >= 3 && (pos - nodes_.front().anchor).len() <= CLOSE_SNAP_RADIUS)
            {
                commitCurve(true);
                return true;
            }

            nodes_.push_back(BezierNode{.anchor = pos, .handle_in = pos, .handle_out = pos});
            is_dragging_ = true;
            return true;
        }

        case MOTION:
        {
            if (is_dragging_ && !nodes_.empty())
            {
                BezierNode& curr = nodes_.back();
                const Vec2F delta = pos - curr.anchor;
                curr.handle_out   = curr.anchor + delta;
                curr.handle_in    = curr.anchor - delta;
                return true;
            }
            return !nodes_.empty();
        }

        case LEFT_BUTTON_RELEASING:
        {
            if (is_dragging_)
            {
                is_dragging_ = false;
                return true;
            }
            return false;
        }

        case RIGHT_BUTTON_PRESSING:
        {
            if (!nodes_.empty())
            {
                commitCurve(false);
                return true;
            }
            return false;
        }

        default:
            return false;
    }
}

inline void BezierTool::render()
{
    if (nodes_.empty())
    {
        return;
    }

    const IRenderer::Color curve_col  = IRenderer::Color::WHITE;
    const IRenderer::Color guide_col  = IRenderer::Color::BLUE;
    const IRenderer::Color handle_col = IRenderer::Color::WHITE;

    // 1. Drawing already constructed curve segments
    const std::vector<Vec2F> built = flattenBezierSpline(nodes_, false);
    for (std::size_t i = 1; i < built.size(); ++i)
    {
        renderer_.renderLine(curve_col, toDocGlobal(built[i - 1]), toDocGlobal(built[i]));
    }

    // 2. Preview next segment to cursor
    if (!is_dragging_ && has_cursor_)
    {
        const BezierNode& last   = nodes_.back();
        Vec2F             target = cursor_local_;
        Vec2F             t_in   = cursor_local_;

        if (nodes_.size() >= 3 && (cursor_local_ - nodes_.front().anchor).len() <= CLOSE_SNAP_RADIUS)
        {
            target = nodes_.front().anchor;
            t_in   = nodes_.front().handle_in;
        }

        constexpr int PREVIEW_STEPS = 20;
        Vec2F         prev          = toDocGlobal(last.anchor);
        for (int s = 1; s <= PREVIEW_STEPS; ++s)
        {
            const float t  = static_cast<float>(s) / static_cast<float>(PREVIEW_STEPS);
            const Vec2F pt = toDocGlobal(evalCubicBezier(last.anchor, last.handle_out, t_in, target, t));
            renderer_.renderLine(guide_col, prev, pt);
            prev = pt;
        }
    }

    // 3. Drawing nodes and tangents
    for (const BezierNode& n : nodes_)
    {
        const Vec2F a  = toDocGlobal(n.anchor);
        const Vec2F hi = toDocGlobal(n.handle_in);
        const Vec2F ho = toDocGlobal(n.handle_out);

        if ((hi - a).len() > 0.5F)
        {
            renderer_.renderLine(guide_col, a, hi);
            renderer_.renderEllipse(handle_col, BoxF{hi.x() - 3.0F, hi.y() - 3.0F, hi.x() + 3.0F, hi.y() + 3.0F});
            renderer_.renderEllipseLines(guide_col, BoxF{hi.x() - 3.0F, hi.y() - 3.0F, hi.x() + 3.0F, hi.y() + 3.0F});
        }

        if ((ho - a).len() > 0.5F)
        {
            renderer_.renderLine(guide_col, a, ho);
            renderer_.renderEllipse(handle_col, BoxF{ho.x() - 3.0F, ho.y() - 3.0F, ho.x() + 3.0F, ho.y() + 3.0F});
            renderer_.renderEllipseLines(guide_col, BoxF{ho.x() - 3.0F, ho.y() - 3.0F, ho.x() + 3.0F, ho.y() + 3.0F});
        }

        const BoxF node_box{a.x() - NODE_HALF_SIZE, a.y() - NODE_HALF_SIZE, a.x() + NODE_HALF_SIZE,
                            a.y() + NODE_HALF_SIZE};
        renderer_.renderRect(handle_col, node_box);
        renderer_.renderRectLines(guide_col, node_box);
    }
}

} // namespace Tools

} // namespace Murugov