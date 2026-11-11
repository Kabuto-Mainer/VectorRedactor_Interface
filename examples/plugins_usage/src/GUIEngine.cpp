#include "GUIEngine.hpp"

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/Package.hpp"
#include "Utils/Event.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_blendmode.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_video.h>
#include <SDL3_gfx/SDL3_gfxPrimitives.h>
#include <cassert>
#include <optional>

#define LOG_SDL_ERR()                                                                      \
    do                                                                                     \
    {                                                                                      \
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "%s\n", SDL_GetError()); \
    } while (0)

#define LOG_NOT_IMPLEMENTED()                                                                                 \
    do                                                                                                        \
    {                                                                                                         \
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "FIXME: %s is not implemented yet", __func__); \
    } while (0)

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    ApplicationCore::instance().registerInterface(
        IID_RENDERER, reinterpret_cast<void*>(static_cast<IRenderer*>(&GUIEngine::instance()))
    );
    ApplicationCore::instance().registerInterface(
        IID_INPUT_PUMP, reinterpret_cast<void*>(static_cast<IInputPump*>(&GUIEngine::instance()))
    );

    return true;
}

void GUIEngine::openWindow(const Vec2F& size)
{
    window_ = SDL_CreateWindow(nullptr, static_cast<int32_t>(size.x()), static_cast<int32_t>(size.y()), 0);
    if (window_ == nullptr)
    {
        LOG_SDL_ERR();
        return;
    }

    renderer_ = SDL_CreateRenderer(window_, nullptr);
    if (renderer_ == nullptr)
    {
        LOG_SDL_ERR();
        return;
    }
}

void GUIEngine::switchRenderColor(const Color& color)
{
    assert(renderer_);

    if (!SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a))
    {
        LOG_SDL_ERR();
        return;
    }
}

void GUIEngine::closeWindow()
{
    SDL_DestroyRenderer(renderer_);
    SDL_DestroyWindow(window_);

    renderer_ = nullptr;
    window_   = nullptr;
}

/* ------------------ public methods ------------------ */

GUIEngine::GUIEngine()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        LOG_SDL_ERR();
        return;
    }
    ApplicationCore::log(ApplicationCore::LogLevel::OK, "Graphics initialized successfully");

    const Vec2F window_size = {1000.0F, 1000.0F};
    openWindow(window_size);
}

GUIEngine::~GUIEngine()
{
    closeWindow();
    SDL_Quit();

    ApplicationCore::log(ApplicationCore::LogLevel::OK, "Graphics deinitialized successfully");
}

GUIEngine& GUIEngine::instance()
{
    static GUIEngine instance;
    return instance;
}

void GUIEngine::renderRect(const Color& color, const BoxF& box, float angle)
{
    if (angle != 0.0F)
    {
        LOG_NOT_IMPLEMENTED();
        return;
    }

    assert(renderer_);

    SDL_FRect rect = {
        .x = box.left(),
        .y = box.top(),
        .w = box.size().x(),
        .h = box.size().y(),
    };

    switchRenderColor(color);

    if (!SDL_RenderFillRect(renderer_, &rect))
    {
        LOG_SDL_ERR();
        return;
    }
}

void GUIEngine::renderEllipse(const Color& color, const BoxF& box, float angle)
{
    assert(renderer_);

    if (angle != 0)
    {
        LOG_NOT_IMPLEMENTED();
        return;
    }

    switchRenderColor(color);

    if (!filledEllipseRGBA(renderer_, box.left(), box.top(), box.size().x(), box.size().y(), color.r, color.g, color.b,
                           color.a))
    {
        LOG_SDL_ERR();
        return;
    }
}

void GUIEngine::update()
{
    assert(renderer_);

    if (!SDL_RenderPresent(renderer_))
    {
        LOG_SDL_ERR();
        return;
    }

    switchRenderColor(Color{});
    if (!SDL_RenderClear(renderer_))
    {
        LOG_SDL_ERR();
        return;
    }
}

void GUIEngine::clear()
{
}

std::optional<Event> GUIEngine::pollEvent()
{
    SDL_Event sdl_event = {};
    if (SDL_PollEvent(&sdl_event))
    {
        switch (sdl_event.type)
        {
            case SDL_EVENT_MOUSE_MOTION:
                return EventMouse(EventMouse::Type::MOTION, Vec2F{sdl_event.motion.x, sdl_event.motion.y});

            // FIXME: all buttons are left button for now
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                return EventMouse(EventMouse::Type::LEFT_BUTTON_PRESSING,
                                  Vec2F{sdl_event.motion.x, sdl_event.motion.y});

            case SDL_EVENT_MOUSE_BUTTON_UP:
                return EventMouse(EventMouse::Type::LEFT_BUTTON_RELEASING,
                                  Vec2F{sdl_event.motion.x, sdl_event.motion.y});

            case SDL_EVENT_QUIT:
                return EventQuit();

            default:
                break;
        }
    }

    return std::nullopt;
}

/* Stubs */

std::optional<IRenderer::TextureID> GUIEngine::loadTexture([[maybe_unused]] const char* path)
{
    LOG_NOT_IMPLEMENTED();
    return static_cast<TextureID>(0);
}

std::optional<IRenderer::FontID> GUIEngine::loadFont([[maybe_unused]] const char* filename)
{
    LOG_NOT_IMPLEMENTED();
    return static_cast<FontID>(0);
}

void GUIEngine::renderRectLines([[maybe_unused]] const Color& color, [[maybe_unused]] const BoxF& box,
                                [[maybe_unused]] float angle)
{
    LOG_NOT_IMPLEMENTED();
}

void GUIEngine::renderEllipseLines([[maybe_unused]] const Color& color, [[maybe_unused]] const BoxF& box,
                                   [[maybe_unused]] float angle)
{
    LOG_NOT_IMPLEMENTED();
}

void GUIEngine::renderLine([[maybe_unused]] const Vec2F& origin, [[maybe_unused]] const Vec2F& end,
                           [[maybe_unused]] const Color& color)
{
    LOG_NOT_IMPLEMENTED();
}

void GUIEngine::renderVector([[maybe_unused]] const Vec2F& origin, [[maybe_unused]] const Vec2F& vector,
                             [[maybe_unused]] const Color& color)
{
    LOG_NOT_IMPLEMENTED();
}

void GUIEngine::renderPixel([[maybe_unused]] const Vec2F& pos, [[maybe_unused]] const Color& color)
{
    LOG_NOT_IMPLEMENTED();
}

void GUIEngine::renderTexture([[maybe_unused]] const TextureID& texture, [[maybe_unused]] const BoxF& box,
                              [[maybe_unused]] float angle, [[maybe_unused]] bool flip_by_oy)
{
    LOG_NOT_IMPLEMENTED();
}

void GUIEngine::renderText([[maybe_unused]] const std::string& text, [[maybe_unused]] const Color& color,
                           [[maybe_unused]] const BoxF& box, [[maybe_unused]] float angle,
                           [[maybe_unused]] const FontID& font_id, [[maybe_unused]] int font_size,
                           [[maybe_unused]] bool flip_by_oy)
{
    LOG_NOT_IMPLEMENTED();
}

void GUIEngine::renderTextPro([[maybe_unused]] const std::string& text, [[maybe_unused]] const Color& color,
                              [[maybe_unused]] const BoxF& box, [[maybe_unused]] const TextFormat& format,
                              [[maybe_unused]] float angle, [[maybe_unused]] bool flip_on_oy)
{
    LOG_NOT_IMPLEMENTED();
}
