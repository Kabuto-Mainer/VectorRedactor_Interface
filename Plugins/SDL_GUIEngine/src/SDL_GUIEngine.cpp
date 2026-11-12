#include "SDL_GUIEngine.hpp"

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/Package.hpp"

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
#include <string>

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
        IID_RENDERER, reinterpret_cast<void*>(static_cast<IRenderer*>(&SDL_GUIEngine::instance()))
    );
    ApplicationCore::instance().registerInterface(
        IID_INPUT_PUMP, reinterpret_cast<void*>(static_cast<IInputPump*>(&SDL_GUIEngine::instance()))
    );

    return true;
}

void SDL_GUIEngine::openWindow(const Vec2F& size)
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

void SDL_GUIEngine::switchRenderColor(const IRenderer::Color& color)
{
    assert(renderer_);

    if (!SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a))
    {
        LOG_SDL_ERR();
        return;
    }
}

void SDL_GUIEngine::closeWindow()
{
    SDL_DestroyRenderer(renderer_);
    SDL_DestroyWindow(window_);

    renderer_ = nullptr;
    window_   = nullptr;
}

/* ------------------ public methods ------------------ */

SDL_GUIEngine::SDL_GUIEngine()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        LOG_SDL_ERR();
        return;
    }
    ApplicationCore::log(ApplicationCore::LogLevel::OK, "graphics initialized successfully");

    const Vec2F window_size = {1000.0F, 1000.0F};
    openWindow(window_size);
}

SDL_GUIEngine::~SDL_GUIEngine()
{
    closeWindow();
    SDL_Quit();

    ApplicationCore::log(ApplicationCore::LogLevel::OK, "graphics deinitialized successfully");
}

SDL_GUIEngine& SDL_GUIEngine::instance()
{
    static SDL_GUIEngine instance;
    return instance;
}

void SDL_GUIEngine::renderRect(const IRenderer::Color& color, const BoxF& box, float angle)
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

void SDL_GUIEngine::renderEllipse(const IRenderer::Color& color, const BoxF& box, float angle)
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

void SDL_GUIEngine::update()
{
    assert(renderer_);

    if (!SDL_RenderPresent(renderer_))
    {
        LOG_SDL_ERR();
        return;
    }

    switchRenderColor(IRenderer::Color{});
    if (!SDL_RenderClear(renderer_))
    {
        LOG_SDL_ERR();
        return;
    }
}

void SDL_GUIEngine::clear()
{
}

std::optional<IInputPump::Event> SDL_GUIEngine::pollEvent()
{
    SDL_Event sdl_event = {};
    if (SDL_PollEvent(&sdl_event))
    {
        switch (sdl_event.type)
        {
            case SDL_EVENT_MOUSE_MOTION:
                return IInputPump::EventMouse(IInputPump::EventMouse::Type::MOTION,
                                              Vec2F{sdl_event.motion.x, sdl_event.motion.y});

            // FIXME: all buttons are left button for now
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                return IInputPump::EventMouse(IInputPump::EventMouse::Type::LEFT_BUTTON_PRESSING,
                                              Vec2F{sdl_event.motion.x, sdl_event.motion.y});

            case SDL_EVENT_MOUSE_BUTTON_UP:
                return IInputPump::EventMouse(IInputPump::EventMouse::Type::LEFT_BUTTON_RELEASING,
                                              Vec2F{sdl_event.motion.x, sdl_event.motion.y});

            case SDL_EVENT_QUIT:
                return IInputPump::EventQuit();

            default:
                break;
        }
    }

    return std::nullopt;
}

/* Stubs */

std::optional<IRenderer::TextureID> SDL_GUIEngine::loadTexture([[maybe_unused]] const std::string& filename)
{
    LOG_NOT_IMPLEMENTED();
    return static_cast<TextureID>(0);
}

std::optional<IRenderer::FontID> SDL_GUIEngine::loadFont([[maybe_unused]] const std::string& filename)
{
    LOG_NOT_IMPLEMENTED();
    return static_cast<FontID>(0);
}

void SDL_GUIEngine::renderRectLines([[maybe_unused]] const IRenderer::Color& color, [[maybe_unused]] const BoxF& box,
                                    [[maybe_unused]] float angle)
{
    LOG_NOT_IMPLEMENTED();
}

void SDL_GUIEngine::renderEllipseLines([[maybe_unused]] const IRenderer::Color& color, [[maybe_unused]] const BoxF& box,
                                       [[maybe_unused]] float angle)
{
    LOG_NOT_IMPLEMENTED();
}

void SDL_GUIEngine::renderLine([[maybe_unused]] const IRenderer::Color& color, [[maybe_unused]] const Vec2F& origin,
                               [[maybe_unused]] const Vec2F& end)
{
    LOG_NOT_IMPLEMENTED();
}

void SDL_GUIEngine::renderVector([[maybe_unused]] const IRenderer::Color& color, [[maybe_unused]] const Vec2F& origin,
                                 [[maybe_unused]] const Vec2F& vector)
{
    LOG_NOT_IMPLEMENTED();
}

void SDL_GUIEngine::renderPixel([[maybe_unused]] const IRenderer::Color& color, [[maybe_unused]] const Vec2F& pos)
{
    LOG_NOT_IMPLEMENTED();
}

void SDL_GUIEngine::renderTexture([[maybe_unused]] const TextureID& texture, [[maybe_unused]] const BoxF& box,
                                  [[maybe_unused]] float angle)
{
    LOG_NOT_IMPLEMENTED();
}

void SDL_GUIEngine::renderText([[maybe_unused]] const IRenderer::Color& color, [[maybe_unused]] const BoxF& box,
                               [[maybe_unused]] const std::string& text, [[maybe_unused]] const TextFormat& format,
                               [[maybe_unused]] float angle)
{
    LOG_NOT_IMPLEMENTED();
}
