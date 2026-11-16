#include "SDL3Renderer.hpp"

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/Vec2F.hpp"

#include <SDL3/SDL.h>
#include <SDL3_gfx/SDL3_gfxPrimitives.h>
#include <SDL3_image/SDL_image.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <numbers>
#include <optional>
#include <vector>

namespace
{

constexpr float WINDOW_SIZE         = 1000.0F;
constexpr int   FONT_PROBE_SIZE     = 16;
constexpr int   ELLIPSE_SEGMENTS    = 72;
constexpr float VECTOR_ARROW_LENGTH = 10.0F;
constexpr float VECTOR_ARROW_ANGLE  = 25.0F * std::numbers::pi_v<float> / 180.0F;
constexpr float RAD_TO_DEG          = 180.0F / std::numbers::pi_v<float>;

void logSdlError(const char* what)
{
    ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "%s: %s", what, SDL_GetError());
}

Vec2F rotate(const Vec2F& vec, float angle)
{
    const float c = std::cos(angle);
    const float s = std::sin(angle);

    return {vec.x() * c - vec.y() * s, vec.x() * s + vec.y() * c};
}

std::vector<Vec2F> rotatedCorners(const BoxF& box, float angle)
{
    const Vec2F        corner_list[] = {box.topLeft(), box.topRight(), box.bottomRight(), box.bottomLeft()};
    const Vec2F        center        = box.center();
    std::vector<Vec2F> corners;

    corners.reserve(4);
    for (const Vec2F& corner : corner_list)
    {
        corners.push_back(center + rotate(corner - center, angle));
    }

    return corners;
}

std::vector<Vec2F> ellipsePoints(const BoxF& box, float angle)
{
    const Vec2F        center = box.center();
    const Vec2F        radius = box.size() / 2.0F;
    std::vector<Vec2F> points;

    points.reserve(ELLIPSE_SEGMENTS);
    for (int i = 0; i < ELLIPSE_SEGMENTS; ++i)
    {
        const float step =
            2.0F * std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(ELLIPSE_SEGMENTS);
        const Vec2F offset = {std::cos(step) * radius.x(), std::sin(step) * radius.y()};

        points.push_back(center + rotate(offset, angle));
    }

    return points;
}

SDL_FRect toSDLRect(const BoxF& box)
{
    return {box.left(), box.top(), box.size().x(), box.size().y()};
}

} // namespace

/* ------------------ private methods ------------------ */

SDL3Renderer::SDL3Renderer()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        logSdlError("SDL_Init failed");
        return;
    }

    if (!TTF_Init())
    {
        logSdlError("TTF_Init failed");
        return;
    }

    ApplicationCore::log(ApplicationCore::LogLevel::OK, "Renderer initialized successfully");

    openWindow(Vec2F{WINDOW_SIZE, WINDOW_SIZE});
}

SDL3Renderer::~SDL3Renderer()
{
    for (auto& texture : textures_)
    {
        SDL_DestroyTexture(texture.second);
    }
    for (auto& font : fonts_)
    {
        for (auto& instance : font.second.instances)
        {
            TTF_CloseFont(instance.second);
        }
    }

    closeWindow();
    TTF_Quit();
    SDL_Quit();

    ApplicationCore::log(ApplicationCore::LogLevel::OK, "Renderer deinitialized successfully");
}

void SDL3Renderer::openWindow(const Vec2F& size)
{
    window_ = SDL_CreateWindow("GDInterface", static_cast<int>(size.x()), static_cast<int>(size.y()), 0);
    if (window_ == nullptr)
    {
        logSdlError("SDL_CreateWindow failed");
        return;
    }

    renderer_ = SDL_CreateRenderer(window_, nullptr);
    if (renderer_ == nullptr)
    {
        logSdlError("SDL_CreateRenderer failed");
        return;
    }

    if (!SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND))
    {
        logSdlError("SDL_SetRenderDrawBlendMode failed");
    }
}

void SDL3Renderer::closeWindow()
{
    SDL_DestroyRenderer(renderer_);
    SDL_DestroyWindow(window_);

    renderer_ = nullptr;
    window_   = nullptr;
}

void SDL3Renderer::setRenderColor(const Color& color)
{
    assert(renderer_);

    if (!SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a))
    {
        logSdlError("SDL_SetRenderDrawColor failed");
    }
}

void SDL3Renderer::drawPolygon(const std::vector<Vec2F>& points, const Color& color, bool filled)
{
    assert(renderer_);

    if (points.size() < 3)
    {
        return;
    }

    std::vector<float> xs;
    std::vector<float> ys;
    xs.reserve(points.size());
    ys.reserve(points.size());

    for (const Vec2F& point : points)
    {
        xs.push_back(point.x());
        ys.push_back(point.y());
    }

    const auto count = static_cast<int>(points.size());
    const bool ok = filled ?
                        filledPolygonRGBA(renderer_, xs.data(), ys.data(), count, color.r, color.g, color.b, color.a) :
                        polygonRGBA(renderer_, xs.data(), ys.data(), count, color.r, color.g, color.b, color.a);
    if (!ok)
    {
        logSdlError("Polygon drawing failed");
    }
}

TTF_Font* SDL3Renderer::fontInstance(FontID font_id, int size)
{
    auto font = fonts_.find(font_id);
    if (font == fonts_.end())
    {
        return nullptr;
    }

    auto& instances = font->second.instances;
    auto  cached    = instances.find(size);
    if (cached != instances.end())
    {
        return cached->second;
    }

    TTF_Font* instance = TTF_OpenFont(font->second.path.c_str(), static_cast<float>(size));
    if (instance != nullptr)
    {
        instances[size] = instance;
    }

    return instance;
}

/* ------------------ public methods ------------------ */

bool SDL3Renderer::isValid() const
{
    return window_ != nullptr && renderer_ != nullptr;
}

/* from IRenderer */

std::optional<IRenderer::TextureID> SDL3Renderer::loadTexture(const std::string& filename)
{
    assert(renderer_);

    if (filename.empty())
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Texture path is empty");
        return std::nullopt;
    }

    SDL_Texture* texture = IMG_LoadTexture(renderer_, filename.c_str());
    if (texture == nullptr)
    {
        logSdlError("Failed to load texture");
        return std::nullopt;
    }

    if (!SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND))
    {
        logSdlError("SDL_SetTextureBlendMode failed");
    }

    const TextureID id = next_texture_id_++;
    textures_[id]      = texture;

    return id;
}

std::optional<IRenderer::FontID> SDL3Renderer::loadFont(const std::string& filename)
{
    if (filename.empty())
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Font filename is empty");
        return std::nullopt;
    }

    TTF_Font* probe = TTF_OpenFont(filename.c_str(), FONT_PROBE_SIZE);
    if (probe == nullptr)
    {
        logSdlError("Failed to load font");
        return std::nullopt;
    }

    const FontID id = next_font_id_++;
    fonts_[id]      = Font{filename, {{FONT_PROBE_SIZE, probe}}};

    return id;
}

void SDL3Renderer::renderRect(const Color& color, const BoxF& box, float angle)
{
    assert(renderer_);

    if (angle == 0.0F)
    {
        const SDL_FRect rect = toSDLRect(box);

        setRenderColor(color);
        if (!SDL_RenderFillRect(renderer_, &rect))
        {
            logSdlError("SDL_RenderFillRect failed");
        }
        return;
    }

    drawPolygon(rotatedCorners(box, angle), color, true);
}

void SDL3Renderer::renderRectLines(const Color& color, const BoxF& box, float angle)
{
    assert(renderer_);

    if (angle == 0.0F)
    {
        const SDL_FRect rect = toSDLRect(box);

        setRenderColor(color);
        if (!SDL_RenderRect(renderer_, &rect))
        {
            logSdlError("SDL_RenderRect failed");
        }
        return;
    }

    drawPolygon(rotatedCorners(box, angle), color, false);
}

void SDL3Renderer::renderTexture(const TextureID& texture, const BoxF& box, float angle)
{
    assert(renderer_);

    auto found = textures_.find(texture);
    if (found == textures_.end())
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Texture %llu is not loaded",
                             static_cast<unsigned long long>(texture));
        return;
    }

    const SDL_FRect dst = toSDLRect(box);
    if (!SDL_RenderTextureRotated(renderer_, found->second, nullptr, &dst, angle * RAD_TO_DEG, nullptr, SDL_FLIP_NONE))
    {
        logSdlError("SDL_RenderTextureRotated failed");
    }
}

void SDL3Renderer::renderEllipse(const Color& color, const BoxF& box, float angle)
{
    assert(renderer_);

    if (angle == 0.0F)
    {
        const Vec2F center = box.center();
        const Vec2F radius = box.size() / 2.0F;

        if (!filledEllipseRGBA(renderer_, center.x(), center.y(), radius.x(), radius.y(), color.r, color.g, color.b,
                               color.a))
        {
            logSdlError("filledEllipseRGBA failed");
        }
        return;
    }

    drawPolygon(ellipsePoints(box, angle), color, true);
}

void SDL3Renderer::renderEllipseLines(const Color& color, const BoxF& box, float angle)
{
    assert(renderer_);

    if (angle == 0.0F)
    {
        const Vec2F center = box.center();
        const Vec2F radius = box.size() / 2.0F;

        if (!ellipseRGBA(renderer_, center.x(), center.y(), radius.x(), radius.y(), color.r, color.g, color.b, color.a))
        {
            logSdlError("ellipseRGBA failed");
        }
        return;
    }

    drawPolygon(ellipsePoints(box, angle), color, false);
}

void SDL3Renderer::renderLine(const Color& color, const Vec2F& origin, const Vec2F& end)
{
    assert(renderer_);

    if (!lineRGBA(renderer_, origin.x(), origin.y(), end.x(), end.y(), color.r, color.g, color.b, color.a))
    {
        logSdlError("lineRGBA failed");
    }
}

void SDL3Renderer::renderVector(const Color& color, const Vec2F& origin, const Vec2F& vector)
{
    assert(renderer_);

    const Vec2F end = origin + vector;
    renderLine(color, origin, end);

    const float length = vector.len();
    if (length == 0.0F)
    {
        return;
    }

    const Vec2F direction = vector / length;
    const float head      = std::min(VECTOR_ARROW_LENGTH, length);

    renderLine(color, end, end - rotate(direction, VECTOR_ARROW_ANGLE) * head);
    renderLine(color, end, end - rotate(direction, -VECTOR_ARROW_ANGLE) * head);
}

void SDL3Renderer::renderPixel(const Color& color, const Vec2F& pos)
{
    assert(renderer_);

    if (!pixelRGBA(renderer_, pos.x(), pos.y(), color.r, color.g, color.b, color.a))
    {
        logSdlError("pixelRGBA failed");
    }
}

void SDL3Renderer::update()
{
    assert(renderer_);

    if (!SDL_RenderPresent(renderer_))
    {
        logSdlError("SDL_RenderPresent failed");
    }

    clear();
}

void SDL3Renderer::clear()
{
    assert(renderer_);

    setRenderColor(Color::BLACK);
    if (!SDL_RenderClear(renderer_))
    {
        logSdlError("SDL_RenderClear failed");
    }
}

void SDL3Renderer::renderText(const Color& color, const BoxF& box, const std::string& text, const TextFormat& format,
                              float angle)
{
    assert(renderer_);

    if (text.empty())
    {
        return;
    }

    /*
     * NOTE: only font and font size from TextFormat are supported for now.
     * Alignment, wrapping and overflow actions are ignored.
     */
    TTF_Font* font = fontInstance(format.font, format.font_size);
    if (font == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Font %llu is not loaded or can not be opened",
                             static_cast<unsigned long long>(format.font));
        return;
    }

    const SDL_Color sdl_color = {color.r, color.g, color.b, color.a};
    SDL_Surface*    surface   = TTF_RenderText_Blended(font, text.c_str(), text.size(), sdl_color);
    if (surface == nullptr)
    {
        logSdlError("TTF_RenderText_Blended failed");
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
    SDL_DestroySurface(surface);
    if (texture == nullptr)
    {
        logSdlError("SDL_CreateTextureFromSurface failed");
        return;
    }

    float text_width  = 0.0F;
    float text_height = 0.0F;
    SDL_GetTextureSize(texture, &text_width, &text_height);

    const Vec2F     center = box.center();
    const SDL_FRect dst    = {center.x() - text_width / 2.0F, center.y() - text_height / 2.0F, text_width, text_height};
    const bool      has_clip = box.size().x() > 0.0F && box.size().y() > 0.0F;
    const SDL_Rect  clip     = {
        static_cast<int>(std::floor(box.left())),
        static_cast<int>(std::floor(box.top())),
        static_cast<int>(std::ceil(box.size().x())),
        static_cast<int>(std::ceil(box.size().y())),
    };

    if (has_clip)
    {
        SDL_SetRenderClipRect(renderer_, &clip);
    }

    if (!SDL_RenderTextureRotated(renderer_, texture, nullptr, &dst, angle * RAD_TO_DEG, nullptr, SDL_FLIP_NONE))
    {
        logSdlError("SDL_RenderTextureRotated failed");
    }

    if (has_clip)
    {
        SDL_SetRenderClipRect(renderer_, nullptr);
    }

    SDL_DestroyTexture(texture);
}
