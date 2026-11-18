#include "SelectorTool.hpp"

#include <cmath>
#include <numbers>
#include <variant>

namespace Yasha::Tools
{

SelectorTool::SelectorTool(Document& document, ToolManager& manager, SelectionOverlay& overlay) :
    document_(document), manager_(manager), overlay_(overlay), last_activation_revision_(manager.activationRevision())
{
}

Vec2F SelectorTool::toDocumentLocal(const Vec2F& global_pos) const
{
    if (const auto* object = dynamic_cast<const TObject*>(&document_))
    {
        return object->mapGlobalToLocal(global_pos);
    }
    return global_pos;
}

bool SelectorTool::processEvent(const IInputPump::Event& event)
{
    if (last_activation_revision_ != manager_.activationRevision())
    {
        gesture_ = Gesture::NONE;
        dragged_id_.reset();
        last_activation_revision_ = manager_.activationRevision();
    }

    if (const auto* key = std::get_if<IInputPump::EventKeyboard>(&event))
    {
        if (key->type() == IInputPump::EventKeyboard::Type::KEY_DOWN &&
            key->key() == IInputPump::EventKeyboard::Key::ESC)
        {
            if (gesture_ != Gesture::NONE)
            {
                return cancel();
            }
            document_.selectShape(std::nullopt);
            return true;
        }
        return false;
    }

    const auto* mouse = std::get_if<IInputPump::EventMouse>(&event);
    if (mouse == nullptr)
    {
        return false;
    }
    switch (mouse->type())
    {
        case IInputPump::EventMouse::Type::LEFT_BUTTON_PRESSING:
            return press(mouse->cursorPos());
        case IInputPump::EventMouse::Type::MOTION:
            return move(mouse->cursorPos());
        case IInputPump::EventMouse::Type::LEFT_BUTTON_RELEASING:
            return release(mouse->cursorPos());
        default:
            return false;
    }
}

bool SelectorTool::press(const Vec2F& global_pos)
{
    if (gesture_ != Gesture::NONE)
    {
        release(global_pos);
    }

    if (overlay_.hitTestRotationHandle(global_pos))
    {
        dragged_id_ = document_.selectedShapeId();
        if (dragged_id_)
        {
            if (auto* shape = document_.findShape(*dragged_id_))
            {
                gesture_ = Gesture::ROTATE;
                start_rotation_ = shape->rotation();
                const Vec2F delta = global_pos - shape->mapLocalToGlobal(Vec2F{});
                previous_cursor_angle_ = std::atan2(delta.y(), delta.x());
                return true;
            }
        }
    }

    dragged_id_ = document_.hitTest(global_pos);
    document_.selectShape(dragged_id_);
    if (dragged_id_)
    {
        if (auto* shape = document_.findShape(*dragged_id_))
        {
            gesture_ = Gesture::MOVE;
            start_cursor_ = toDocumentLocal(global_pos);
            start_translation_ = shape->translation();
        }
    }
    return true;
}

bool SelectorTool::move(const Vec2F& global_pos)
{
    if (gesture_ == Gesture::NONE || !dragged_id_)
    {
        return false;
    }
    auto* shape = document_.findShape(*dragged_id_);
    if (shape == nullptr)
    {
        gesture_ = Gesture::NONE;
        dragged_id_.reset();
        return false;
    }
    if (gesture_ == Gesture::MOVE)
    {
        shape->setTranslation(start_translation_ + toDocumentLocal(global_pos) - start_cursor_);
    }
    else
    {
        const Vec2F delta = global_pos - shape->mapLocalToGlobal(Vec2F{});
        if (delta.len2() > 1.0e-6F)
        {
            const float angle = std::atan2(delta.y(), delta.x());
            shape->rotate(std::remainder(angle - previous_cursor_angle_, 2.0F * std::numbers::pi_v<float>));
            previous_cursor_angle_ = angle;
        }
    }
    return true;
}

bool SelectorTool::release(const Vec2F& global_pos)
{
    if (gesture_ == Gesture::NONE)
    {
        return false;
    }
    move(global_pos);
    gesture_ = Gesture::NONE;
    dragged_id_.reset();
    return true;
}

bool SelectorTool::cancel()
{
    if (gesture_ == Gesture::NONE)
    {
        return false;
    }
    if (dragged_id_)
    {
        if (auto* shape = document_.findShape(*dragged_id_))
        {
            if (gesture_ == Gesture::MOVE)
            {
                shape->setTranslation(start_translation_);
            }
            else
            {
                shape->setRotation(start_rotation_);
            }
        }
    }
    gesture_ = Gesture::NONE;
    dragged_id_.reset();
    return true;
}

} // namespace Yasha::Tools
