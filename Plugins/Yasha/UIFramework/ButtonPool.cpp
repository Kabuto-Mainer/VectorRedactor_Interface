#include "UIFramework.hpp"

#include <utility>

namespace UI
{

ButtonPool::ButtonPool(const Vec2F& pos, ButtonPoolDirection direction) : direction_(direction)
{
    setBox(BoxF(pos, pos));
}

void ButtonPool::add(std::unique_ptr<Button> button)
{
    if (button == nullptr)
    {
        return;
    }

    addChild(std::move(button));
    layout();
}

bool ButtonPool::remove(TObject::ID button_id)
{
    for (auto it = children().begin(); it != children().end(); ++it)
    {
        if ((*it)->id() == button_id)
        {
            children().erase(it);
            layout();
            return true;
        }
    }
    return false;
}

bool ButtonPool::processEvent(const IInputPump::Event& event)
{
    return processEventChildren(event);
}

void ButtonPool::render()
{
    renderChildren();
}

void ButtonPool::layout()
{
    Vec2F offset{};
    for (auto& child : children())
    {
        auto& button = static_cast<Button&>(*child);
        button.setBox(BoxF(offset, offset + Vec2F(BUTTON_SIZE)));
        offset = offset +
                 (direction_ == ButtonPoolDirection::HORIZONTAL ? Vec2F(BUTTON_SIZE, 0.0F) : Vec2F(0.0F, BUTTON_SIZE));
    }

    const float count = static_cast<float>(children().size());
    const Vec2F size  = direction_ == ButtonPoolDirection::HORIZONTAL ? Vec2F(BUTTON_SIZE * count, BUTTON_SIZE) :
                                                                        Vec2F(BUTTON_SIZE, BUTTON_SIZE * count);
    const Vec2F pos   = box().topLeft();
    setBox(BoxF(pos, pos + (children().empty() ? Vec2F{} : size)));
}

} // namespace UI
