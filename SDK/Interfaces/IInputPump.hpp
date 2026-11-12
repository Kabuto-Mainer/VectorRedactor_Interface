#ifndef IINPUT_PUMP_HPP
#define IINPUT_PUMP_HPP

#include "Interfaces/IID.hpp"
#include "Utils/Vec2F.hpp"

#include <optional>
#include <variant>

constexpr static const IID IID_INPUT_PUMP = 1;

class IInputPump
{
public:
    class EventMouse;
    class EventKeyboard;
    class EventQuit;
    using Event = std::variant<EventMouse, EventKeyboard, EventQuit /*, ... */>;

public:
    IInputPump()          = default;
    virtual ~IInputPump() = default;

    virtual std::optional<Event> pollEvent() = 0;
};

class IInputPump::EventMouse
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

class IInputPump::EventKeyboard
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

class IInputPump::EventQuit
{
public:
    EventQuit()  = default;
    ~EventQuit() = default;
};

/* ======================== Implementation ======================== */

/* EventMouse */

inline IInputPump::EventMouse::EventMouse(Type type, const Vec2F& cursor_pos) : type_(type), cursor_pos_(cursor_pos)
{
}

inline IInputPump::EventMouse::Type IInputPump::EventMouse::type() const
{
    return type_;
}

inline Vec2F IInputPump::EventMouse::cursorPos() const
{
    return cursor_pos_;
}

#endif
