#ifndef PHOTOSHOP_EVENT_HPP
#define PHOTOSHOP_EVENT_HPP

#include "SDK/Utils/Vec2F.hpp"

#include <variant>

enum class EventType
{
    MOUSE_DOWN,
    MOUSE_UP,
    MOUSE_MOVE,
    KEY_PRESSED,
    KEY_RELEASED,
    WINDOW_CLOSE,
};

enum class MouseButton
{
    LEFT,
    RIGHT,
    MIDDLE,
};

enum class KeyCode
{
    UNKNOWN,
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
    DIGIT_0,
    DIGIT_1,
    DIGIT_2,
    DIGIT_3,
    DIGIT_4,
    DIGIT_5,
    DIGIT_6,
    DIGIT_7,
    DIGIT_8,
    DIGIT_9,
    SPACE,
    ESCAPE,
    ENTER,
    TAB,
    BACKSPACE,
    DELETE_KEY,
    INSERT,
    HOME,
    END,
    PAGE_UP,
    PAGE_DOWN,
    LEFT,
    RIGHT,
    UP,
    DOWN,
    LEFT_SHIFT,
    RIGHT_SHIFT,
    LEFT_CONTROL,
    RIGHT_CONTROL,
    LEFT_ALT,
    RIGHT_ALT,
    LEFT_SUPER,
    RIGHT_SUPER,
    CAPS_LOCK,
    NUM_LOCK,
    SCROLL_LOCK,
    PRINT_SCREEN,
    PAUSE,
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
    GRAVE,
    MINUS,
    EQUAL,
    LEFT_BRACKET,
    RIGHT_BRACKET,
    BACKSLASH,
    SEMICOLON,
    APOSTROPHE,
    COMMA,
    PERIOD,
    SLASH,
    KEYPAD_0,
    KEYPAD_1,
    KEYPAD_2,
    KEYPAD_3,
    KEYPAD_4,
    KEYPAD_5,
    KEYPAD_6,
    KEYPAD_7,
    KEYPAD_8,
    KEYPAD_9,
    KEYPAD_DECIMAL,
    KEYPAD_DIVIDE,
    KEYPAD_MULTIPLY,
    KEYPAD_SUBTRACT,
    KEYPAD_ADD,
    KEYPAD_ENTER,
    KEYPAD_EQUAL,
    KEYBOARD_MENU,
};

// Реализуйте в адаптере выбранной графической библиотеки: сопоставьте её коды
// клавиш с KeyCode через switch и верните UNKNOWN для неподдерживаемых клавиш.
namespace Input
{

KeyCode convertKey(int library_key);

} // namespace Input

struct MouseEvent
{
    Vec2F       pos;
    MouseButton button = MouseButton::LEFT;
};

struct KeyModifiers
{
    bool left_shift    = false;
    bool right_shift   = false;
    bool left_control  = false;
    bool right_control = false;
    bool left_alt      = false;
    bool right_alt     = false;
    bool left_super    = false;
    bool right_super   = false;
};

struct KeyboardEvent
{
    KeyCode      key_code  = KeyCode::UNKNOWN;
    bool         is_repeat = false;
    KeyModifiers modifiers;
};

struct WindowEvent
{
};

struct Event
{
    EventType                                            type = EventType::MOUSE_MOVE;
    std::variant<MouseEvent, KeyboardEvent, WindowEvent> data = MouseEvent{};

    static Event createMouseEvent(EventType type, const Vec2F& pos, MouseButton button)
    {
        return {
            type,
            MouseEvent{pos, button},
        };
    }

    static Event createKeyEvent(EventType type, KeyCode code, bool repeat, KeyModifiers modifiers = {})
    {
        KeyboardEvent keyboard{code, repeat, modifiers};
        return {
            type,
            keyboard,
        };
    }

    static Event createWindowCloseEvent()
    {
        return {
            EventType::WINDOW_CLOSE,
            WindowEvent{},
        };
    }
};

#endif
