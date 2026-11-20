#pragma once

#include "Interfaces/IDocument.hpp"
#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IToolManager.hpp"
#include "Utils/TObject.hpp"
#include "Utils/Vec2F.hpp"

#include <optional>
#include <variant>

namespace Kabuto
{

namespace Tools
{

/**
 * Removes the topmost shape under the left-clicked cursor position.
 *
 * A click on empty canvas is still consumed: the tool is the owner of canvas
 * clicks while it is active, so no other lower object can react to them.
 */
class DeleteShapeTool final : public IToolManager::ATool
{
public:
    DeleteShapeTool(ID id, IDocument& document);
    ~DeleteShapeTool() override = default;

    DeleteShapeTool(const DeleteShapeTool&)            = delete;
    DeleteShapeTool(DeleteShapeTool&&)                 = delete;
    DeleteShapeTool& operator=(const DeleteShapeTool&) = delete;
    DeleteShapeTool& operator=(DeleteShapeTool&&)      = delete;

    ID   id() const override;
    bool processEvent(const IInputPump::Event& event) override;

private:
    ID         id_;
    IDocument& document_;
}; // class DeleteShapeTool

/* ====================== Implementation ====================== */

inline DeleteShapeTool::DeleteShapeTool(ID id, IDocument& document) : id_(id), document_(document)
{
}

inline IToolManager::ATool::ID DeleteShapeTool::id() const
{
    return id_;
}

inline bool DeleteShapeTool::processEvent(const IInputPump::Event& event)
{
    const auto* mouse = std::get_if<IInputPump::EventMouse>(&event);
    if (mouse == nullptr || mouse->type() != IInputPump::EventMouse::Type::LEFT_BUTTON_PRESSING)
    {
        return false;
    }

    const Vec2F position = mouse->cursorPos();
    const std::optional<TObject::ID> shape_id = document_.hitTest(position);
    if (shape_id.has_value())
    {
        document_.removeShape(*shape_id);
    }

    return true;
}

}; // namespace Tools

}; // namespace Kabuto
