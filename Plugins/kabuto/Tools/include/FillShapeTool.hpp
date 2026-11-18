#pragma once

#include "Components.hpp"
#include "IPalette.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IToolManager.hpp"
#include "ShapeLookup.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/TObject.hpp"
#include "Utils/Vec2F.hpp"

#include <optional>
#include <variant>

namespace Kabuto
{

namespace Tools
{

/**
 * Fills the topmost shape under the cursor with the colour selected in the palette.
 *
 * Shapes that do not expose a ColorComponent are left untouched, but the click
 * is still consumed because the tool owns canvas clicks while it is active.
 */
class FillShapeTool final : public IToolManager::ATool
{
public:
    FillShapeTool(ID id, IDocument& document, IPalette& palette, const BoxF& ui_box);
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
    BoxF       ui_box_;
}; // class FillShapeTool

/* ====================== Implementation ====================== */

inline FillShapeTool::FillShapeTool(ID id, IDocument& document, IPalette& palette, const BoxF& ui_box) :
    id_(id), document_(document), palette_(palette), ui_box_(ui_box)
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
    if (ui_box_.contains(position))
    {
        return false;
    }

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
