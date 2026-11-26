#pragma once

#include "Interfaces/IDocument.hpp"
#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/IToolManager.hpp"
#include "Utils/TObject.hpp"
#include "Utils/Vec2F.hpp"
#include "export/Components.hpp"
#include "export/IPalette.hpp"

#include <optional>
#include <variant>

namespace Kabuto
{

namespace Tools
{

/**
 * Paints the topmost shape under the cursor with the current palette colour.
 *
 * Shapes without a ColorComponent are ignored, but the click is consumed so
 * that nothing below the canvas reacts to it.
 */
class SetColorTool final : public IToolManager::ATool
{
public:
    static constexpr IToolManager::ATool::ID ID    = 1;

    SetColorTool(IDocument& document, IPalette& palette);
    ~SetColorTool() override = default;

    SetColorTool(const SetColorTool&)            = delete;
    SetColorTool(SetColorTool&&)                 = delete;
    SetColorTool& operator=(const SetColorTool&) = delete;
    SetColorTool& operator=(SetColorTool&&)      = delete;

    IToolManager::ATool::ID id() const override;
    bool                    processEvent(const IInputPump::Event& event) override;

private:
    bool paintShape(const Vec2F& global_pos);

    IDocument& document_;
    IPalette&  palette_;
}; // class SetColorTool

/**
 * IDocument::accept() is const-only, while the tool naturally needs to modify
 * the shape. The document owns its shapes, therefore casting away constness
 * here does not violate the ownership model.
 */
class SetColorVisitor final : public IDocument::Visitor
{
public:
    SetColorVisitor(TObject::ID shape_id, IRenderer::Color color);
    ~SetColorVisitor() = default;

    SetColorVisitor(const SetColorVisitor&)            = delete;
    SetColorVisitor(SetColorVisitor&&)                 = delete;
    SetColorVisitor& operator=(const SetColorVisitor&) = delete;
    SetColorVisitor& operator=(SetColorVisitor&&)      = delete;

    bool visit(IDocument::TShape& shape) override;
    bool visit(const IDocument::TShape& shape) override;

    bool painted() const;

private:
    TObject::ID      shape_id_;
    IRenderer::Color color_;
    bool             painted_ = false;
}; // class SetColorVisitor

/* ====================== Implementation ====================== */

inline SetColorTool::SetColorTool(IDocument& document, IPalette& palette) :
    document_(document), palette_(palette)
{
}

inline IToolManager::ATool::ID SetColorTool::id() const
{
    return ID;
}

inline bool SetColorTool::processEvent(const IInputPump::Event& event)
{
    const auto* mouse = std::get_if<IInputPump::EventMouse>(&event);
    if (mouse == nullptr || mouse->type() != IInputPump::EventMouse::Type::LEFT_BUTTON_PRESSING)
    {
        return false;
    }

    paintShape(mouse->cursorPos());
    return true;
}

inline bool SetColorTool::paintShape(const Vec2F& global_pos)
{
    const std::optional<TObject::ID> shape_id = document_.hitTest(global_pos);
    if (!shape_id.has_value())
    {
        return false;
    }

    SetColorVisitor visitor(*shape_id, palette_.color());
    document_.accept(visitor);

    return visitor.painted();
}

inline SetColorVisitor::SetColorVisitor(TObject::ID shape_id, IRenderer::Color color) :
    shape_id_(shape_id), color_(color)
{
}

inline bool SetColorVisitor::visit(IDocument::TShape& shape)
{
    if (shape.id() != shape_id_)
    {
        return true;
    }

    IDocument::AShapeComponent* component = nullptr;
    if (shape.queryComponent(Document::ColorComponent::COID, &component) == IDocument::TShape::Error::SUCCESS &&
        component != nullptr)
    {
        static_cast<Document::ColorComponent*>(component)->setColor(color_);
        painted_ = true;
    }

    return false;
}

inline bool SetColorVisitor::visit(const IDocument::TShape& shape)
{
    return visit(const_cast<IDocument::TShape&>(shape));
}

inline bool SetColorVisitor::painted() const
{
    return painted_;
}

}; // namespace Tools

}; // namespace Kabuto
