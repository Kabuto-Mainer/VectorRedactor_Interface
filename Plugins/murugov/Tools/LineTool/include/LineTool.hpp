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
#include <memory>
#include <variant>

namespace Murugov
{

namespace Tools
{

constexpr IToolManager::ATool::ID LINE_TOOL_ID = 11;

class LineColorComponent : public IDocument::AShapeComponent
{
public:
    constexpr static const ID COID = 4;

    IRenderer::Color color() const { return color_; }
    void             setColor(const IRenderer::Color& color) { color_ = color; }

protected:
    IRenderer::Color color_ = IRenderer::Color::WHITE;
};

/* ====================== LineShape ====================== */

class LineShape final : public IDocument::TShape, public LineColorComponent
{
public:
    static constexpr IDocument::TShape::TypeID LINE_SHAPE_ID = 104;
    static constexpr float                     HIT_TOLERANCE = 6.0F;

    LineShape(const Vec2F& p1_doc, const Vec2F& p2_doc, IRenderer::Color color = IRenderer::Color::WHITE);
    ~LineShape() override = default;

    bool processEvent(const IInputPump::Event& event) override;
    void render() override;

    bool   hitTest(Vec2F const& pos) const override;
    BoxF   axesAlignedBoundingBox() const override;
    TypeID tid() const override { return LINE_SHAPE_ID; }

    Error queryComponent(IDocument::AShapeComponent::ID coid, IDocument::AShapeComponent** component) override;
    Error queryComponent(IDocument::AShapeComponent::ID     coid,
                         IDocument::AShapeComponent const** component) const override;

private:
    Vec2F local_p1_;
    Vec2F local_p2_;
};

/* ====================== LineTool ====================== */

class LineTool final : public IToolManager::ATool
{
public:
    LineTool(ID tool_id, IDocument& document, IRenderer& renderer);
    ~LineTool() override = default;

    LineTool(const LineTool&)            = delete;
    LineTool(LineTool&&)                 = delete;
    LineTool& operator=(const LineTool&) = delete;
    LineTool& operator=(LineTool&&)      = delete;

    ID   id() const override { return id_; }
    bool processEvent(const IInputPump::Event& event) override;
    void render();

private:
    static constexpr float MIN_LINE_LEN = 3.0F;

    Vec2F            toDocLocal(const Vec2F& global_pos) const;
    Vec2F            toDocGlobal(const Vec2F& local_pos) const;
    IRenderer::Color currentColor() const;

    ID         id_;
    IDocument& document_;
    IRenderer& renderer_;

    bool  is_drawing_ = false;
    Vec2F start_local_;
    Vec2F current_local_;
};

/* ====================== Implementation ====================== */

inline LineShape::LineShape(const Vec2F& p1_doc, const Vec2F& p2_doc, IRenderer::Color color)
{
    setColor(color);

    const float min_x = std::min(p1_doc.x(), p2_doc.x());
    const float max_x = std::max(p1_doc.x(), p2_doc.x());
    const float min_y = std::min(p1_doc.y(), p2_doc.y());
    const float max_y = std::max(p1_doc.y(), p2_doc.y());

    const float w = std::max(max_x - min_x, 4.0F);
    const float h = std::max(max_y - min_y, 4.0F);
    const Vec2F center{(min_x + max_x) * 0.5F, (min_y + max_y) * 0.5F};

    setTranslation(center);
    setScaling({w, h});

    local_p1_ = {(p1_doc.x() - center.x()) / w, (p1_doc.y() - center.y()) / h};
    local_p2_ = {(p2_doc.x() - center.x()) / w, (p2_doc.y() - center.y()) / h};
}

inline bool LineShape::processEvent(const IInputPump::Event& /*event*/)
{
    return false;
}

inline void LineShape::render()
{
    IRenderer* renderer = nullptr;
    if (ApplicationCore::instance().queryInterface(IID_RENDERER, reinterpret_cast<void**>(&renderer)) !=
            ApplicationCore::Error::SUCCESS ||
        renderer == nullptr)
    {
        return;
    }

    renderer->renderLine(color(), mapLocalToGlobal(local_p1_), mapLocalToGlobal(local_p2_));
}

inline bool LineShape::hitTest(Vec2F const& pos) const
{
    const Vec2F a  = mapLocalToGlobal(local_p1_);
    const Vec2F b  = mapLocalToGlobal(local_p2_);
    const Vec2F ab = b - a;
    const float l2 = ab.len2();

    float t = 0.0F;
    if (l2 > 1e-5F)
    {
        const Vec2F ap = pos - a;
        t              = std::clamp((ap.x() * ab.x() + ap.y() * ab.y()) / l2, 0.0F, 1.0F);
    }

    const Vec2F closest = a + ab * t;
    return (pos - closest).len() <= HIT_TOLERANCE;
}

inline BoxF LineShape::axesAlignedBoundingBox() const
{
    const Vec2F g1 = transform().transformPoint(local_p1_);
    const Vec2F g2 = transform().transformPoint(local_p2_);

    return BoxF{std::min(g1.x(), g2.x()), std::min(g1.y(), g2.y()),
                std::max(g1.x(), g2.x()), std::max(g1.y(), g2.y())};
}

inline IDocument::TShape::Error LineShape::queryComponent(IDocument::AShapeComponent::ID coid,
                                                          IDocument::AShapeComponent**   component)
{
    if (component == nullptr)
    {
        return Error::INVALID_ARGS;
    }

    if (coid == LineColorComponent::COID)
    {
        *component = static_cast<LineColorComponent*>(this);
        return Error::SUCCESS;
    }

    *component = nullptr;
    return Error::COMPONENT_NOT_FOUND;
}

inline IDocument::TShape::Error LineShape::queryComponent(IDocument::AShapeComponent::ID     coid,
                                                          IDocument::AShapeComponent const** component) const
{
    if (component == nullptr)
    {
        return Error::INVALID_ARGS;
    }

    if (coid == LineColorComponent::COID)
    {
        *component = static_cast<const LineColorComponent*>(this);
        return Error::SUCCESS;
    }

    *component = nullptr;
    return Error::COMPONENT_NOT_FOUND;
}

inline LineTool::LineTool(ID tool_id, IDocument& document, IRenderer& renderer) :
    id_(tool_id), document_(document), renderer_(renderer)
{
}

inline Vec2F LineTool::toDocLocal(const Vec2F& global_pos) const
{
    if (const auto* node = dynamic_cast<const TObject*>(&document_))
    {
        return node->mapGlobalToLocal(global_pos);
    }
    return global_pos;
}

inline Vec2F LineTool::toDocGlobal(const Vec2F& local_pos) const
{
    if (const auto* node = dynamic_cast<const TObject*>(&document_))
    {
        return node->mapLocalToGlobal(local_pos);
    }
    return local_pos;
}

inline IRenderer::Color LineTool::currentColor() const
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

inline bool LineTool::processEvent(const IInputPump::Event& event)
{
    if (const auto* key = std::get_if<IInputPump::EventKeyboard>(&event))
    {
        if (key->type() == IInputPump::EventKeyboard::Type::KEY_DOWN &&
            key->key() == IInputPump::EventKeyboard::Key::ESC && is_drawing_)
        {
            is_drawing_ = false;
            return true;
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
        {
            is_drawing_    = true;
            start_local_   = toDocLocal(mouse->cursorPos());
            current_local_ = start_local_;
            return true;
        }

        case MOTION:
        {
            if (!is_drawing_)
            {
                return false;
            }
            current_local_ = toDocLocal(mouse->cursorPos());
            return true;
        }

        case LEFT_BUTTON_RELEASING:
        {
            if (!is_drawing_)
            {
                return false;
            }
            is_drawing_ = false;

            if ((current_local_ - start_local_).len() >= MIN_LINE_LEN)
            {
                auto shape = std::make_unique<LineShape>(start_local_, current_local_, currentColor());
                document_.addShape(std::move(shape));
            }
            return true;
        }

        default:
            return false;
    }
}

inline void LineTool::render()
{
    if (!is_drawing_)
    {
        return;
    }

    const Vec2F g1 = toDocGlobal(start_local_);
    const Vec2F g2 = toDocGlobal(current_local_);

    renderer_.renderLine(IRenderer::Color::BLUE, g1, g2);

    constexpr float R = 3.0F;
    renderer_.renderRect(currentColor(), BoxF{g1.x() - R, g1.y() - R, g1.x() + R, g1.y() + R});
    renderer_.renderRect(currentColor(), BoxF{g2.x() - R, g2.y() - R, g2.x() + R, g2.y() + R});
}

} // namespace Tools

} // namespace Murugov