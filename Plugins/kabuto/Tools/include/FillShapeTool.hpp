#pragma once

#include "Components.hpp"
#include "IPalette.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IToolManager.hpp"
#include "ShapeLookup.hpp"
#include "Utils/TObject.hpp"
#include "Utils/Vec2F.hpp"

#include <optional>
#include <variant>

namespace Kabuto
{

namespace Tools
{

/**
 * Fills the topmost shape under the cursor with the palette colour.
 *
 * Shapes without a ColorComponent are ignored, but the click is consumed so
 * that nothing below the canvas reacts to it.
 */
class FillShapeTool final : public IToolManager::ATool
{
public:
    FillShapeTool(ID id, IDocument& document, IPalette& palette);
    ~FillShapeTool() override = default;

    FillShapeTool(const FillShapeTool&)            = delete;
    FillShapeTool(FillShapeTool&&)                 = delete;
    FillShapeTool& operator=(const FillShapeTool&) = delete;
    FillShapeTool& operator=(FillShapeTool&&)      = delete;

    ID   id() const override;
    bool processEvent(const IInputPump::Event& event) override;

private:
    bool fillShape(const Vec2F& global_pos);

    ID         id_;
    IDocument& document_;
    IPalette&  palette_;
}; // class FillShapeTool

/* ====================== Implementation ====================== */

inline FillShapeTool::FillShapeTool(ID id, IDocument& document, IPalette& palette) :
    id_(id), document_(document), palette_(palette)
{
}

inline IToolManager::ATool::ID FillShapeTool::id() const
{
    return id_;
}

inline bool FillShapeTool::processEvent(const IInputPump::Event& event)
{
    const auto* mouse = std::get_if<IInputPump::EventMouse>(&event);
    if (mouse == nullptr || mouse->type() != IInputPump::EventMouse::Type::LEFT_BUTTON_PRESSING)
    {
        return false;
    }

    const Vec2F position = mouse->cursorPos();
    fillShape(position);
    return true;
}

inline bool FillShapeTool::fillShape(const Vec2F& global_pos)
{
    const std::optional<TObject::ID> shape_id = document_.hitTest(global_pos);
    if (!shape_id.has_value())
    {
        return false;
    }

    IDocument::TShape* shape = findShape(document_, *shape_id);
    if (shape == nullptr)
    {
        return false;
    }

    IDocument::AShapeComponent* component = nullptr;
    if (shape->queryComponent(Document::ColorComponent::COID, &component) != IDocument::TShape::Error::SUCCESS ||
        component == nullptr)
    {
        return false;
    }

    static_cast<Document::ColorComponent*>(component)->setColor(palette_.color());
    return true;
}

}; // namespace Tools

}; // namespace Kabuto
