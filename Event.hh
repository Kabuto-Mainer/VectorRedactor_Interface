#pragma once


#include "CommonClass.hh"
#include <cstdint>

namespace Input
{

enum class EventKind
{
    Mouse,
    Key,
}; // enum class EventKind

class Event;

namespace Mouse
{

enum class Button
{
    Left,
    Middle,
    Right
};

enum class EventKind
{
    Press,
    UnPress,
    Move,
    Wheel
};

struct Data
{
    EventKind kind_;
    Button    but_;
    Vec2f     pos_;
    float     wheelX_;
    float     wheelY_;
}; // struct Data

}; // namespace Mouse

namespace Keyboard
{

enum class EventKind
{
    Press,
    UnPress
}; // enum class EventKind

enum class Modifier : uint64_t
{
    None   = 0x0000u,
    LShift = 0x0001u,
    RShift = 0x0002u,
    LAlt   = 0x0004u,
    RAlt   = 0x0008u,
    LCtrl  = 0x0010u,
    RCtrl  = 0x0020u
}; // enum class Modifier

enum class Key
{
    Unknown = 0,
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    Escape, Space, Enter, Tab, Backspace,
    Left, Right, Up, Down,
    LShift, RShift, LCtrl, RCtrl, LAlt, RAlt,
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
}; // enum class Key

struct Data
{
    EventKind kind_;
    Key       key_;
    uint64_t  mod_;
}; // struct Data

}; // namespace Keyboard

class Event
{
public:
    virtual ~Event() = default;
    virtual bool is(EventKind kind) const = 0;
    virtual Mouse::Data getMouse() const = 0;
    virtual Keyboard::Data getKeyboard() const = 0;
}; // class Event

}; // namespace Input
