#include "SDL3Engine.hpp"

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IID.hpp"
#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/Package.hpp"
#include "Utils/Vec2F.hpp"

#include <SDL3/SDL.h>
#include <optional>

namespace
{

using EventKeyboard = IInputPump::EventKeyboard;
using EventMouse    = IInputPump::EventMouse;
using Event         = IInputPump::Event;
using ModifiersMask = EventKeyboard::ModifiersMask;

std::optional<EventMouse::Type> toMouseButtonType(const SDL_MouseButtonEvent& button, bool pressed)
{
    using Type = EventMouse::Type;

    switch (button.button)
    {
        case SDL_BUTTON_LEFT:
            return pressed ? Type::LEFT_BUTTON_PRESSING : Type::LEFT_BUTTON_RELEASING;
        case SDL_BUTTON_RIGHT:
            return pressed ? Type::RIGHT_BUTTON_PRESSING : Type::RIGHT_BUTTON_RELEASING;
        case SDL_BUTTON_MIDDLE:
            return pressed ? Type::MIDDLE_BUTTON_PRESSING : Type::MIDDLE_BUTTON_RELEASING;
        default:
            return std::nullopt;
    }
}

ModifiersMask toModifiers(SDL_Keymod modifiers)
{
    using Modifier = EventKeyboard::Modifier;

    ModifiersMask mask = 0;
    if (modifiers & SDL_KMOD_LSHIFT)
    {
        mask |= EventKeyboard::modifierToMask(Modifier::LEFT_SHIFT);
    }
    if (modifiers & SDL_KMOD_RSHIFT)
    {
        mask |= EventKeyboard::modifierToMask(Modifier::RIGHT_SHIFT);
    }
    if (modifiers & SDL_KMOD_LCTRL)
    {
        mask |= EventKeyboard::modifierToMask(Modifier::LEFT_CTRL);
    }
    if (modifiers & SDL_KMOD_RCTRL)
    {
        mask |= EventKeyboard::modifierToMask(Modifier::RIGHT_CTRL);
    }
    if (modifiers & SDL_KMOD_LALT)
    {
        mask |= EventKeyboard::modifierToMask(Modifier::LEFT_ALT);
    }
    if (modifiers & SDL_KMOD_RALT)
    {
        mask |= EventKeyboard::modifierToMask(Modifier::RIGHT_ALT);
    }
    if (modifiers & SDL_KMOD_LGUI)
    {
        mask |= EventKeyboard::modifierToMask(Modifier::LEFT_META);
    }
    if (modifiers & SDL_KMOD_RGUI)
    {
        mask |= EventKeyboard::modifierToMask(Modifier::RIGHT_META);
    }

    return mask;
}

std::optional<EventKeyboard::Key> toKey(SDL_Keycode keycode)
{
    using Key = EventKeyboard::Key;

    if (keycode >= SDLK_A && keycode <= SDLK_Z)
    {
        return static_cast<Key>(static_cast<int>(Key::A) + (keycode - SDLK_A));
    }
    if (keycode >= SDLK_1 && keycode <= SDLK_9)
    {
        return static_cast<Key>(static_cast<int>(Key::NUM_1) + (keycode - SDLK_1));
    }

    switch (keycode)
    {
        case SDLK_0:
            return Key::NUM_0;

        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            return Key::ENTER;
        case SDLK_ESCAPE:
            return Key::ESC;
        case SDLK_BACKSPACE:
            return Key::BACKSPACE;
        case SDLK_TAB:
            return Key::TAB;
        case SDLK_SPACE:
            return Key::SPACE;

        case SDLK_MINUS:
            return Key::MINUS;
        case SDLK_EQUALS:
            return Key::EQUALS;
        case SDLK_LEFTBRACKET:
            return Key::LEFTBRACKET;
        case SDLK_RIGHTBRACKET:
            return Key::RIGHTBRACKET;
        case SDLK_BACKSLASH:
            return Key::BACKSLASH;
        case SDLK_SEMICOLON:
            return Key::SEMICOLON;
        case SDLK_APOSTROPHE:
            return Key::APOSTROPHE;
        case SDLK_COMMA:
            return Key::COMMA;
        case SDLK_PERIOD:
            return Key::PERIOD;
        case SDLK_SLASH:
            return Key::SLASH;

        case SDLK_CAPSLOCK:
            return Key::CAPSLOCK;

        case SDLK_F1:
            return Key::F1;
        case SDLK_F2:
            return Key::F2;
        case SDLK_F3:
            return Key::F3;
        case SDLK_F4:
            return Key::F4;
        case SDLK_F5:
            return Key::F5;
        case SDLK_F6:
            return Key::F6;
        case SDLK_F7:
            return Key::F7;
        case SDLK_F8:
            return Key::F8;
        case SDLK_F9:
            return Key::F9;
        case SDLK_F10:
            return Key::F10;
        case SDLK_F11:
            return Key::F11;
        case SDLK_F12:
            return Key::F12;

        case SDLK_PRINTSCREEN:
            return Key::PRINTSCREEN;
        case SDLK_SCROLLLOCK:
            return Key::SCROLLLOCK;
        case SDLK_PAUSE:
            return Key::PAUSE;
        case SDLK_INSERT:
            return Key::INSERT;
        case SDLK_HOME:
            return Key::HOME;
        case SDLK_PAGEUP:
            return Key::PAGEUP;
        case SDLK_DELETE:
            return Key::DEL;
        case SDLK_END:
            return Key::END;
        case SDLK_PAGEDOWN:
            return Key::PAGEDOWN;
        case SDLK_RIGHT:
            return Key::RIGHT;
        case SDLK_LEFT:
            return Key::LEFT;
        case SDLK_DOWN:
            return Key::DOWN;
        case SDLK_UP:
            return Key::UP;

        default:
            return std::nullopt;
    }
}

std::optional<Event> convertEvent(const SDL_Event& sdl_event)
{
    switch (sdl_event.type)
    {
        case SDL_EVENT_MOUSE_MOTION:
            return EventMouse(EventMouse::Type::MOTION, Vec2F{sdl_event.motion.x, sdl_event.motion.y});

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
        {
            const bool pressed = sdl_event.type == SDL_EVENT_MOUSE_BUTTON_DOWN;
            const auto type    = toMouseButtonType(sdl_event.button, pressed);
            if (!type.has_value())
            {
                return std::nullopt;
            }

            return EventMouse(type.value(), Vec2F{sdl_event.button.x, sdl_event.button.y});
        }

        case SDL_EVENT_MOUSE_WHEEL:
        {
            if (sdl_event.wheel.y == 0.0F)
            {
                return std::nullopt;
            }

            const auto type =
                sdl_event.wheel.y > 0.0F ? EventMouse::Type::WHEEL_FORWARD : EventMouse::Type::WHEEL_BACKWARD;

            return EventMouse(type, Vec2F{sdl_event.wheel.mouse_x, sdl_event.wheel.mouse_y});
        }

        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
        {
            if (sdl_event.key.repeat)
            {
                return std::nullopt;
            }

            const auto key = toKey(sdl_event.key.key);
            if (!key.has_value())
            {
                return std::nullopt;
            }

            return EventKeyboard(key.value(), toModifiers(sdl_event.key.mod));
        }

        case SDL_EVENT_QUIT:
            return IInputPump::EventQuit();

        default:
            return std::nullopt;
    }
}

} // namespace

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    SDL3Engine& engine = SDL3Engine::instance();
    if (!engine.isValid())
    {
        return false;
    }

    if (ApplicationCore::instance().registerInterface(IID_RENDERER, static_cast<IRenderer*>(&engine)) !=
        ApplicationCore::Error::SUCCESS)
    {
        return false;
    }

    if (ApplicationCore::instance().registerInterface(IID_INPUT_PUMP, static_cast<IInputPump*>(&engine)) !=
        ApplicationCore::Error::SUCCESS)
    {
        return false;
    }

    return true;
}

SDL3Engine::SDL3Engine()
{
    ApplicationCore::log(ApplicationCore::LogLevel::OK, "Input pump initialized successfully");
}

SDL3Engine::~SDL3Engine()
{
    ApplicationCore::log(ApplicationCore::LogLevel::OK, "Input pump deinitialized successfully");
}

SDL3Engine& SDL3Engine::instance()
{
    static SDL3Engine instance;
    return instance;
}

std::optional<Event> SDL3Engine::pollEvent()
{
    SDL_Event sdl_event = {};
    while (SDL_PollEvent(&sdl_event))
    {
        auto event = convertEvent(sdl_event);
        if (event.has_value())
        {
            return event;
        }
    }

    return std::nullopt;
}
