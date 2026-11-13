#ifndef IINPUT_PUMP_HPP
#define IINPUT_PUMP_HPP

#include "Interfaces/IID.hpp"
#include "Utils/Vec2F.hpp"

#include <cstdint>
#include <optional>
#include <variant>

constexpr static const IID IID_INPUT_PUMP = 1;

/* IInputPump */

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

/* EventMouse */

class IInputPump::EventMouse
{
public:
    enum class Type;

    constexpr EventMouse(Type type, const Vec2F& cursor_pos);

    constexpr Type  type() const;
    constexpr Vec2F cursorPos() const;

private:
    Type  type_;
    Vec2F cursor_pos_;
};

enum class IInputPump::EventMouse::Type
{
    MOTION,
    WHEEL_FORWARD,
    WHEEL_BACKWARD,
    LEFT_BUTTON_PRESSING,
    LEFT_BUTTON_RELEASING,
    RIGHT_BUTTON_PRESSING,
    RIGHT_BUTTON_RELEASING,
    MIDDLE_BUTTON_PRESSING,
    MIDDLE_BUTTON_RELEASING,
};

constexpr IInputPump::EventMouse::EventMouse(Type type, const Vec2F& cursor_pos) : type_(type), cursor_pos_(cursor_pos)
{
}

constexpr IInputPump::EventMouse::Type IInputPump::EventMouse::type() const
{
    return type_;
}

constexpr Vec2F IInputPump::EventMouse::cursorPos() const
{
    return cursor_pos_;
}

/* EventKeyboard */

class IInputPump::EventKeyboard
{
public:
    using ModifiersMask = uint32_t;
    enum class Modifier : ModifiersMask;
    enum class Type;
    enum class Key;

    static constexpr ModifiersMask modifierToMask(Modifier modifier);

    constexpr EventKeyboard(Key key);
    constexpr EventKeyboard(Key key, ModifiersMask modifiers_mask);
    template<typename... Tmodifier>
    constexpr EventKeyboard(Key key, Tmodifier... modifiers);

    template<typename... Tmodifier>
    constexpr bool hasModifiers(Tmodifier... modifiers);

private:
    Key           key_;
    ModifiersMask modifiers_mask_ = 0;
};

constexpr IInputPump::EventKeyboard::ModifiersMask IInputPump::EventKeyboard::modifierToMask(Modifier modifier)
{
    return static_cast<ModifiersMask>(modifier);
}

enum class Key
{
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,

    NUM_1,
    NUM_2,
    NUM_3,
    NUM_4,
    NUM_5,
    NUM_6,
    NUM_7,
    NUM_8,
    NUM_9,
    NUM_0,

    ENTER,
    ESC,
    BACKSPACE,
    TAB,
    SPACE,

    MINUS,
    EQUALS,
    LEFTBRACKET,
    RIGHTBRACKET,
    BACKSLASH,
    NONUSHASH,
    SEMICOLON,
    APOSTROPHE,
    COMMA,
    PERIOD,
    SLASH,

    CAPSLOCK,

    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,

    PRINTSCREEN,
    SCROLLLOCK,
    PAUSE,
    INSERT,
    HOME,
    PAGEUP,
    DEL,
    END,
    PAGEDOWN,
    RIGHT,
    LEFT,
    DOWN,
    UP,
};

enum class IInputPump::EventKeyboard::Type
{
    KEY_DOWN,
    KEY_UP,
};

enum class IInputPump::EventKeyboard::Modifier : IInputPump::EventKeyboard::ModifiersMask
{
    NONE = 0,

    LEFT_CTRL   = 1 << 0,
    LEFT_SHIFT  = 1 << 1,
    LEFT_ALT    = 1 << 2,
    LEFT_META   = 1 << 3,
    RIGHT_CTRL  = 1 << 4,
    RIGHT_SHIFT = 1 << 5,
    RIGHT_ALT   = 1 << 6,
    RIGHT_META  = 1 << 7,
};

constexpr IInputPump::EventKeyboard::EventKeyboard(Key key) : key_(key)
{
}

constexpr IInputPump::EventKeyboard::EventKeyboard(Key key, ModifiersMask modifiers_mask) :
    key_(key), modifiers_mask_(modifiers_mask)
{
}

template<typename... Tmodifier>
constexpr IInputPump::EventKeyboard::EventKeyboard(Key key, Tmodifier... modifiers) :
    key_(key), modifiers_mask_((modifierToMask(modifiers) | ...))
{
}

template<typename... Tmodifier>
constexpr bool IInputPump::EventKeyboard::hasModifiers(Tmodifier... modifiers)
{
    return ((modifiers_mask_ & modifierToMask(modifiers)) && ...);
}

/* EventQuit */

class IInputPump::EventQuit
{
public:
    constexpr EventQuit() = default;
};

#endif
