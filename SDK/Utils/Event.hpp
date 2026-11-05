#ifndef EVENT_HPP
#define EVENT_HPP

#include "Utils/Vec2F.hpp"

#include <variant>

class MouseEvent
{
public:
    enum class Type
    {
        MOVED,
        LEFT_BUTTON_PRESSED,
        LEFT_BUTTON_RELEASED,
        RIGHT_BUTTON_PRESSED,
        RIGHT_BUTTON_RELEASED,
        MIDDLE_BUTTON_PRESSING,
        MIDDLE_BUTTON_RELEASING,
    };

    MouseEvent(Type type, const Vec2F& cursor_pos);
    ~MouseEvent();

    Type  type() const;
    Vec2F cursorPos() const;

private:
    Type  type_{};
    Vec2F cursor_pos_{};
};

class KeyboardEvent
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
    KeyboardEvent();
    ~KeyboardEvent();
};

class CloseEvent
{
public:
    CloseEvent();
    ~CloseEvent();
};

using Event = std::variant<MouseEvent, KeyboardEvent, CloseEvent /*, ... */>;

/* ======================== Implementation ======================== */

/* MouseEvent */

inline MouseEvent::MouseEvent(Type const type, const Vec2F& cursor_pos) : type_{type}, cursor_pos_{cursor_pos}
{
}

inline MouseEvent::~MouseEvent() = default;

inline MouseEvent::Type MouseEvent::type() const
{
    return type_;
}

inline Vec2F MouseEvent::cursorPos() const
{
    return cursor_pos_;
}

/* KeyboardEvent */
// TBD

/* CloseEvent */

inline CloseEvent::CloseEvent()  = default;
inline CloseEvent::~CloseEvent() = default;

#endif
