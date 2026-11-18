#pragma once

#include "Geometry.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IToolManager.hpp"
#include "SelectionState.hpp"
#include "ShapeLookup.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/TObject.hpp"
#include "Utils/Vec2F.hpp"

#include <memory>
#include <optional>
#include <variant>
#include <vector>

namespace Kabuto
{

namespace Tools
{

/**
 * Selects shapes by clicking and by dragging a marquee rectangle.
 *
 * Plain click replaces the selection with the topmost shape under the cursor.
 * Shift-click toggles a shape. Dragging from empty canvas draws a marquee and
 * selects every shape whose axes-aligned bounding box intersects it. Holding
 * Shift while starting a marquee adds the result to the existing selection.
 */
class SelectShapeTool final : public IToolManager::ATool
{
public:
    SelectShapeTool(ID id, IDocument& document, TObject& document_view, const BoxF& ui_box,
                    std::shared_ptr<SelectionState> state);
    ~SelectShapeTool() override = default;

    SelectShapeTool(const SelectShapeTool&)            = delete;
    SelectShapeTool(SelectShapeTool&&)                 = delete;
    SelectShapeTool& operator=(const SelectShapeTool&) = delete;
    SelectShapeTool& operator=(SelectShapeTool&&)      = delete;

    ID   id() const override;
    bool processEvent(const IInputPump::Event& event) override;

private:
    static constexpr float MIN_MARQUEE_SIZE = 2.0F;

    bool handleKey(const IInputPump::EventKeyboard& key);
    bool handleMouse(const IInputPump::EventMouse& mouse);

    bool beginPress(const Vec2F& global_pos);
    bool updateDrag(const Vec2F& global_pos);
    bool endPress(const Vec2F& global_pos);

    void selectInMarquee(const BoxF& box);

private:
    ID                              id_;
    IDocument&                      document_;
    TObject&                        document_view_;
    BoxF                            ui_box_;
    std::shared_ptr<SelectionState> state_;

    Vec2F anchor_;
    bool  pressed_          = false;
    bool  marquee_          = false;
    bool  marquee_additive_ = false;
    bool  shift_held_       = false;
}; // class SelectShapeTool

/* ====================== Implementation ====================== */

inline SelectShapeTool::SelectShapeTool(ID id, IDocument& document, TObject& document_view, const BoxF& ui_box,
                                        std::shared_ptr<SelectionState> state) :
    id_(id), document_(document), document_view_(document_view), ui_box_(ui_box), state_(std::move(state))
{
}

inline IToolManager::ATool::ID SelectShapeTool::id() const
{
    return id_;
}

inline bool SelectShapeTool::processEvent(const IInputPump::Event& event)
{
    if (const auto* key = std::get_if<IInputPump::EventKeyboard>(&event))
    {
        return handleKey(*key);
    }

    if (const auto* mouse = std::get_if<IInputPump::EventMouse>(&event))
    {
        return handleMouse(*mouse);
    }

    return false;
}

inline bool SelectShapeTool::handleKey(const IInputPump::EventKeyboard& key)
{
    using Modifier = IInputPump::EventKeyboard::Modifier;

    // IInputPump reports modifiers through a non-const query; the event itself is read-only here.
    auto& keyboard = const_cast<IInputPump::EventKeyboard&>(key);
    shift_held_    = keyboard.hasModifiers(Modifier::LEFT_SHIFT) || keyboard.hasModifiers(Modifier::RIGHT_SHIFT);

    if (key.type() == IInputPump::EventKeyboard::Type::KEY_DOWN && key.key() == IInputPump::EventKeyboard::Key::ESC)
    {
        state_->setMarquee(std::nullopt);
        pressed_ = false;
        marquee_ = false;

        if (state_->empty())
        {
            return false;
        }

        state_->clear();
        return true;
    }

    return false;
}

inline bool SelectShapeTool::handleMouse(const IInputPump::EventMouse& mouse)
{
    using enum IInputPump::EventMouse::Type;

    switch (mouse.type())
    {
        case LEFT_BUTTON_PRESSING:
            return beginPress(mouse.cursorPos());

        case MOTION:
            return updateDrag(mouse.cursorPos());

        case LEFT_BUTTON_RELEASING:
            return endPress(mouse.cursorPos());

        default:
            return false;
    }
}

inline bool SelectShapeTool::beginPress(const Vec2F& global_pos)
{
    if (ui_box_.contains(global_pos))
    {
        return false;
    }

    pressed_ = true;
    marquee_ = false;

    const std::optional<TObject::ID> hit = document_.hitTest(global_pos);
    if (hit.has_value())
    {
        if (shift_held_)
        {
            state_->toggle(*hit);
        }
        else
        {
            state_->selectOnly(*hit);
        }

        return true;
    }

    marquee_additive_ = shift_held_;
    if (!marquee_additive_)
    {
        state_->clear();
    }

    anchor_  = globalToDocument(document_view_, global_pos);
    marquee_ = true;
    state_->setMarquee(BoxF(anchor_, anchor_));

    return true;
}

inline bool SelectShapeTool::updateDrag(const Vec2F& global_pos)
{
    if (!pressed_ || !marquee_)
    {
        return false;
    }

    const Vec2F current = globalToDocument(document_view_, global_pos);
    state_->setMarquee(boxFromCorners(anchor_, current));

    return true;
}

inline bool SelectShapeTool::endPress(const Vec2F& global_pos)
{
    if (!pressed_)
    {
        return false;
    }

    if (marquee_)
    {
        const Vec2F current  = globalToDocument(document_view_, global_pos);
        const BoxF  box      = boxFromCorners(anchor_, current);
        const bool  is_click = box.size().x() < MIN_MARQUEE_SIZE && box.size().y() < MIN_MARQUEE_SIZE;

        if (!is_click)
        {
            if (!marquee_additive_)
            {
                state_->clear();
            }

            selectInMarquee(box);
        }

        state_->setMarquee(std::nullopt);
    }

    pressed_ = false;
    marquee_ = false;

    return true;
}

inline void SelectShapeTool::selectInMarquee(const BoxF& box)
{
    for (IDocument::TShape* shape : collectShapes(document_))
    {
        if (shape == nullptr)
        {
            continue;
        }

        if (boxesOverlap(shape->axesAlignedBoundingBox(), box))
        {
            state_->add(shape->id());
        }
    }
}

}; // namespace Tools

}; // namespace Kabuto
