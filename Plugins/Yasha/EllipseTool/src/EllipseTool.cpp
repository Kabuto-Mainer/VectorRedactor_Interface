#include "EllipseTool.hpp"

#include "EllipseShape.hpp"

#include <algorithm>
#include <cmath>
#include <variant>

namespace Yasha::Tools
{

EllipseTool::EllipseTool(IDocument& document) : document_(document)
{
}

Vec2F EllipseTool::toLocal(const Vec2F& global_pos) const
{
    if (const auto* object = dynamic_cast<const TObject*>(&document_))
    {
        return object->mapGlobalToLocal(global_pos);
    }
    return global_pos;
}

bool EllipseTool::processEvent(const IInputPump::Event& event)
{
    if (const auto* key = std::get_if<IInputPump::EventKeyboard>(&event))
    {
        if (dragging_ &&
            key->type() == IInputPump::EventKeyboard::Type::KEY_DOWN &&
            key->key() == IInputPump::EventKeyboard::Key::ESC)
        {
            cancelDrag();
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
            return beginDrag(mouse->cursorPos());
        case IInputPump::EventMouse::Type::MOTION:
            return updateDrag(mouse->cursorPos());
        case IInputPump::EventMouse::Type::LEFT_BUTTON_RELEASING:
            return endDrag(mouse->cursorPos());
        default:
            return false;
    }
}

bool EllipseTool::beginDrag(const Vec2F& global_pos)
{
    if (dragging_)
    {
        cancelDrag();
    }
    anchor_   = toLocal(global_pos);
    dragging_ = true;
    return true;
}

bool EllipseTool::updateDrag(const Vec2F& global_pos)
{
    if (!dragging_)
    {
        return false;
    }

    const Vec2F current = toLocal(global_pos);
    const Vec2F min{std::min(anchor_.x(), current.x()), std::min(anchor_.y(), current.y())};
    const Vec2F max{std::max(anchor_.x(), current.x()), std::max(anchor_.y(), current.y())};
    const Vec2F size = max - min;

    if (preview_ == nullptr)
    {
        if (size.x() < MIN_DRAG_SIZE || size.y() < MIN_DRAG_SIZE)
        {
            return true;
        }
        createShape((min + max) / 2.0F, size);
        return preview_ != nullptr;
    }

    preview_->setScaling(size / 2.0F);
    preview_->setTranslation((min + max) / 2.0F);
    return true;
}

bool EllipseTool::endDrag(const Vec2F& global_pos)
{
    if (!dragging_)
    {
        return false;
    }

    updateDrag(global_pos);
    if (preview_ != nullptr && (std::abs(preview_->scaling().x()) * 2.0F < MIN_DRAG_SIZE ||
                                std::abs(preview_->scaling().y()) * 2.0F < MIN_DRAG_SIZE))
    {
        cancelDrag();
    }
    preview_  = nullptr;
    dragging_ = false;
    return true;
}

void EllipseTool::cancelDrag()
{
    if (preview_ != nullptr)
    {
        document_.removeShape(preview_);
    }
    preview_  = nullptr;
    dragging_ = false;
}

void EllipseTool::createShape(const Vec2F& center, const Vec2F& size)
{
    auto* shape = new EllipseShape(size);
    shape->setTranslation(center);
    document_.addShape(shape);
    if (shape->parent() != nullptr)
    {
        preview_ = shape;
    }
    else
    {
        delete shape;
    }
}

} // namespace Yasha::Tools
