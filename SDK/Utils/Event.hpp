#ifndef EVENT_HPP
#define EVENT_HPP

#include "Utils/Vec2F.hpp"

#include <variant>

class EventMouse
{
public:
    enum class Type
    {
        MOTION,
        LEFT_BUTTON_PRESSING,
        LEFT_BUTTON_RELEASING,
        RIGHT_BUTTON_PRESSING,
        RIGHT_BUTTON_RELEASING,
        MIDDLE_BUTTON_PRESSING,
        MIDDLE_BUTTON_RELEASING,
    };

    EventMouse(Type type, const Vec2F& cursor_pos);
    ~EventMouse() = default;

    Type  type() const;
    Vec2F cursorPos() const;

private:
    Type  type_;
    Vec2F cursor_pos_;
};

class EventKeyboard
{
public:
    enum class Type
    {
        // ...
    };

    enum class Key
    {
        // ...
    };
    // Maybe string comparing?
    // "C-A", "C-B" instead enum
    //
    EventKeyboard()  = default;
    ~EventKeyboard() = default;
};

class EventQuit
{
public:
    EventQuit()  = default;
    ~EventQuit() = default;
};

using Event = std::variant<EventMouse, EventKeyboard, EventQuit /*, ... */>;

/* ======================== Implementation ======================== */

/* EventMouse */

EventMouse::EventMouse(Type type, const Vec2F& cursor_pos) : type_(type), cursor_pos_(cursor_pos)
{
}

EventMouse::Type EventMouse::type() const
{
    return type_;
}

Vec2F EventMouse::cursorPos() const
{
    return cursor_pos_;
}

#endif
