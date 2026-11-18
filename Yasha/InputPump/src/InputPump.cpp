#include "InputPump.hpp"

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/Package.hpp"

#include <raylib.h>
#include <utility>

namespace
{

IInputPump::EventKeyboard::ModifiersMask keyboardModifiers()
{
    using Keyboard               = IInputPump::EventKeyboard;
    Keyboard::ModifiersMask mask = 0;
    const auto              add  = [&mask](int ray_key, Keyboard::Modifier modifier)
    {
        if (IsKeyDown(ray_key))
        {
            mask |= Keyboard::modifierToMask(modifier);
        }
    };
    add(KEY_LEFT_CONTROL, Keyboard::Modifier::LEFT_CTRL);
    add(KEY_RIGHT_CONTROL, Keyboard::Modifier::RIGHT_CTRL);
    add(KEY_LEFT_SHIFT, Keyboard::Modifier::LEFT_SHIFT);
    add(KEY_RIGHT_SHIFT, Keyboard::Modifier::RIGHT_SHIFT);
    add(KEY_LEFT_ALT, Keyboard::Modifier::LEFT_ALT);
    add(KEY_RIGHT_ALT, Keyboard::Modifier::RIGHT_ALT);
    add(KEY_LEFT_SUPER, Keyboard::Modifier::LEFT_META);
    add(KEY_RIGHT_SUPER, Keyboard::Modifier::RIGHT_META);
    return mask;
}

using Key = IInputPump::EventKeyboard::Key;

std::optional<Key> mapKey(int ray_key)
{
    if (ray_key >= KEY_A && ray_key <= KEY_Z)
    {
        return static_cast<Key>(static_cast<int>(Key::A) + ray_key - KEY_A);
    }
    if (ray_key >= KEY_ONE && ray_key <= KEY_NINE)
    {
        return static_cast<Key>(static_cast<int>(Key::NUM_1) + ray_key - KEY_ONE);
    }
    if (ray_key == KEY_ZERO)
    {
        return Key::NUM_0;
    }
    if (ray_key >= KEY_F1 && ray_key <= KEY_F12)
    {
        return static_cast<Key>(static_cast<int>(Key::F1) + ray_key - KEY_F1);
    }

    static constexpr std::pair<int, Key> keys[] = {
        {        KEY_ENTER,        Key::ENTER},
        {       KEY_ESCAPE,          Key::ESC},
        {    KEY_BACKSPACE,    Key::BACKSPACE},
        {          KEY_TAB,          Key::TAB},
        {        KEY_SPACE,        Key::SPACE},
        {        KEY_MINUS,        Key::MINUS},
        {        KEY_EQUAL,       Key::EQUALS},
        { KEY_LEFT_BRACKET,  Key::LEFTBRACKET},
        {KEY_RIGHT_BRACKET, Key::RIGHTBRACKET},
        {    KEY_BACKSLASH,    Key::BACKSLASH},
        {    KEY_SEMICOLON,    Key::SEMICOLON},
        {   KEY_APOSTROPHE,   Key::APOSTROPHE},
        {        KEY_COMMA,        Key::COMMA},
        {       KEY_PERIOD,       Key::PERIOD},
        {        KEY_SLASH,        Key::SLASH},
        {    KEY_CAPS_LOCK,     Key::CAPSLOCK},
        { KEY_PRINT_SCREEN,  Key::PRINTSCREEN},
        {  KEY_SCROLL_LOCK,   Key::SCROLLLOCK},
        {        KEY_PAUSE,        Key::PAUSE},
        {       KEY_INSERT,       Key::INSERT},
        {         KEY_HOME,         Key::HOME},
        {      KEY_PAGE_UP,       Key::PAGEUP},
        {       KEY_DELETE,          Key::DEL},
        {          KEY_END,          Key::END},
        {    KEY_PAGE_DOWN,     Key::PAGEDOWN},
        {        KEY_RIGHT,        Key::RIGHT},
        {         KEY_LEFT,         Key::LEFT},
        {         KEY_DOWN,         Key::DOWN},
        {           KEY_UP,           Key::UP},
    };
    for (const auto& [code, key] : keys)
    {
        if (ray_key == code)
        {
            return key;
        }
    }
    return std::nullopt;
}

} // namespace

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    auto& core = ApplicationCore::instance();
    void* existing = nullptr;
    if (core.queryInterface(IID_INPUT_PUMP, &existing) == ApplicationCore::Error::SUCCESS ||
        core.queryInterface(IID_RENDERER, &existing) != ApplicationCore::Error::SUCCESS || !IsWindowReady())
    {
        return false;
    }
    return core.registerInterface(IID_INPUT_PUMP, static_cast<IInputPump*>(&RaylibInputPump::instance())) ==
           ApplicationCore::Error::SUCCESS;
}

RaylibInputPump& RaylibInputPump::instance()
{
    static RaylibInputPump pump;
    return pump;
}

void RaylibInputPump::collectEvents()
{
    if (WindowShouldClose() && !quit_emitted_)
    {
        events_.emplace_back(EventQuit{});
        quit_emitted_ = true;
        ApplicationCore::instance().stop();
    }

    const ::Vector2 mouse = GetMousePosition();
    const Vec2F     pos{mouse.x, mouse.y};
    if (pos != previous_mouse_pos_)
    {
        events_.emplace_back(EventMouse{EventMouse::Type::MOTION, pos});
        previous_mouse_pos_ = pos;
    }

    constexpr std::pair<int, EventMouse::Type> mouse_buttons[] = {
        {  MOUSE_BUTTON_LEFT,   EventMouse::Type::LEFT_BUTTON_PRESSING},
        { MOUSE_BUTTON_RIGHT,  EventMouse::Type::RIGHT_BUTTON_PRESSING},
        {MOUSE_BUTTON_MIDDLE, EventMouse::Type::MIDDLE_BUTTON_PRESSING},
    };
    constexpr EventMouse::Type releases[] = {
        EventMouse::Type::LEFT_BUTTON_RELEASING,
        EventMouse::Type::RIGHT_BUTTON_RELEASING,
        EventMouse::Type::MIDDLE_BUTTON_RELEASING,
    };
    for (std::size_t index = 0; index < std::size(mouse_buttons); ++index)
    {
        if (IsMouseButtonPressed(mouse_buttons[index].first))
        {
            events_.emplace_back(EventMouse{mouse_buttons[index].second, pos});
        }
        if (IsMouseButtonReleased(mouse_buttons[index].first))
        {
            events_.emplace_back(EventMouse{releases[index], pos});
        }
    }

    const float wheel = GetMouseWheelMove();
    if (wheel != 0.0F)
    {
        events_.emplace_back(
            EventMouse{wheel > 0.0F ? EventMouse::Type::WHEEL_FORWARD : EventMouse::Type::WHEEL_BACKWARD, pos}
        );
    }

    const auto modifiers = keyboardModifiers();
    for (int ray_key = KEY_SPACE; ray_key <= KEY_KB_MENU; ++ray_key)
    {
        const auto key = mapKey(ray_key);
        if (!key)
        {
            continue;
        }
        if (IsKeyPressed(ray_key))
        {
            events_.emplace_back(EventKeyboard{EventKeyboard::Type::KEY_DOWN, *key, modifiers});
        }
        if (IsKeyReleased(ray_key))
        {
            events_.emplace_back(EventKeyboard{EventKeyboard::Type::KEY_UP, *key, modifiers});
        }
    }
}

std::optional<IInputPump::Event> RaylibInputPump::pollEvent()
{
    if (!IsWindowReady())
    {
        return std::nullopt;
    }
    collectEvents();
    if (events_.empty())
    {
        return std::nullopt;
    }
    Event event = std::move(events_.front());
    events_.pop_front();
    return event;
}
