#include "SDL3Engine.hpp"

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IID.hpp"
#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/Package.hpp"
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
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Kabuto
{

namespace Engine
{

namespace
{

constexpr int   LOGICAL_WIDTH       = 1'920;
constexpr int   LOGICAL_HEIGHT      = 1'080;
constexpr float DESKTOP_USAGE       = 0.9F;
constexpr int   FONT_PROBE_SIZE     = 16;
constexpr int   ELLIPSE_SEGMENTS    = 72;
constexpr float VECTOR_ARROW_LENGTH = 10.0F;
constexpr float VECTOR_ARROW_ANGLE  = 25.0F * std::numbers::pi_v<float> / 180.0F;
constexpr float RAD_TO_DEG          = 180.0F / std::numbers::pi_v<float>;

/*
 * SDL3_gfx's ellipse primitive computes its scanlines with Sint32 and becomes
 * wrong for radii above ~1024 (the overscan squares overflow). Large ellipses
 * are therefore drawn as polygons instead.
 */
constexpr float MAX_ELLIPSE_PRIMITIVE_RADIUS = 512.0F;

void logSdlError(const char* what)
{
    ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "%s: %s", what, SDL_GetError());
}

/*
 * SDL3_gfx primitives set the renderer draw blend mode themselves (NONE for
 * opaque colors, BLEND otherwise). Restore the previous state afterwards so
 * the engine's rendering is not affected by an earlier gfx call.
 */
class RenderBlendModeGuard final
{
public:
    explicit RenderBlendModeGuard(SDL_Renderer* renderer) : renderer_(renderer)
    {
        if (renderer_ == nullptr || !SDL_GetRenderDrawBlendMode(renderer_, &blend_mode_))
        {
            logSdlError("SDL_GetRenderDrawBlendMode failed");
            renderer_ = nullptr;
        }
    }

    ~RenderBlendModeGuard()
    {
        if (renderer_ != nullptr && !SDL_SetRenderDrawBlendMode(renderer_, blend_mode_))
        {
            logSdlError("SDL_SetRenderDrawBlendMode failed");
        }
    }

    RenderBlendModeGuard(const RenderBlendModeGuard&)            = delete;
    RenderBlendModeGuard& operator=(const RenderBlendModeGuard&) = delete;

private:
    SDL_Renderer* renderer_   = nullptr;
    SDL_BlendMode blend_mode_ = SDL_BLENDMODE_NONE;
};

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

Vec2F initialWindowSize()
{
    SDL_Rect            usable  = {};
    const SDL_DisplayID display = SDL_GetPrimaryDisplay();

    if (display == 0 || !SDL_GetDisplayUsableBounds(display, &usable) || usable.w <= 0 || usable.h <= 0)
    {
        return {static_cast<float>(LOGICAL_WIDTH), static_cast<float>(LOGICAL_HEIGHT)};
    }

    const float width_scale  = static_cast<float>(usable.w) * DESKTOP_USAGE / static_cast<float>(LOGICAL_WIDTH);
    const float height_scale = static_cast<float>(usable.h) * DESKTOP_USAGE / static_cast<float>(LOGICAL_HEIGHT);
    const float scale        = std::min({width_scale, height_scale, 1.0F});

    return {std::floor(static_cast<float>(LOGICAL_WIDTH) * scale),
            std::floor(static_cast<float>(LOGICAL_HEIGHT) * scale)};
}

/* ------------------ text helpers ------------------ */

constexpr int MIN_FONT_SIZE = 1;

constexpr std::string_view TEXT_ELLIPSIS = "...";

struct TextLine
{
    std::string text;
    int         width  = 0;
    int         height = 0;
};

struct TextBlock
{
    std::vector<TextLine> lines;
    int                   line_height  = 0;
    float                 line_advance = 0.0F;
    int                   width        = 0;
    int                   height       = 0;
};

int fontLineHeight(TTF_Font* font)
{
    return std::max(TTF_GetFontHeight(font), 1);
}

bool measureText(TTF_Font* font, std::string_view text, int& width, int& height)
{
    if (text.empty())
    {
        width  = 0;
        height = fontLineHeight(font);
        return true;
    }

    return TTF_GetStringSize(font, text.data(), text.size(), &width, &height);
}

std::size_t utf8CodepointLength(unsigned char lead)
{
    if ((lead & 0x80U) == 0U)
    {
        return 1;
    }
    if ((lead & 0xE0U) == 0xC0U)
    {
        return 2;
    }
    if ((lead & 0xF0U) == 0xE0U)
    {
        return 3;
    }
    if ((lead & 0xF8U) == 0xF0U)
    {
        return 4;
    }

    return 1;
}

std::vector<std::string> splitParagraphs(const std::string& text)
{
    std::vector<std::string> paragraphs;
    std::size_t              start = 0;

    while (true)
    {
        const std::size_t end       = text.find('\n', start);
        std::string       paragraph = text.substr(start, end == std::string::npos ? std::string::npos : end - start);

        if (!paragraph.empty() && paragraph.back() == '\r')
        {
            paragraph.pop_back();
        }

        paragraphs.push_back(std::move(paragraph));

        if (end == std::string::npos)
        {
            break;
        }

        start = end + 1;
    }

    return paragraphs;
}

std::vector<std::string> wrapParagraph(TTF_Font* font, const std::string& paragraph, int max_width)
{
    std::vector<std::string> lines;

    if (max_width <= 0 || paragraph.empty())
    {
        lines.push_back(paragraph);
        return lines;
    }

    std::size_t position = 0;

    while (position < paragraph.size())
    {
        while (position < paragraph.size() && (paragraph[position] == ' ' || paragraph[position] == '\t'))
        {
            ++position;
        }

        if (position >= paragraph.size())
        {
            break;
        }

        const std::size_t remaining = paragraph.size() - position;
        int               width     = 0;
        std::size_t       fits      = remaining;

        if (!TTF_MeasureString(font, paragraph.c_str() + position, remaining, max_width, &width, &fits) || fits == 0U)
        {
            fits = utf8CodepointLength(static_cast<unsigned char>(paragraph[position]));
        }

        if (fits >= remaining)
        {
            lines.push_back(paragraph.substr(position));
            break;
        }

        const std::size_t break_position = paragraph.rfind(' ', position + fits);
        if (break_position != std::string::npos && break_position >= position)
        {
            lines.push_back(paragraph.substr(position, break_position - position));
            position = break_position + 1;
        }
        else
        {
            lines.push_back(paragraph.substr(position, fits));
            position += fits;
        }
    }

    if (lines.empty())
    {
        lines.emplace_back();
    }

    return lines;
}

std::string ellipsize(TTF_Font* font, const std::string& text, int max_width)
{
    int width  = 0;
    int height = 0;

    if (!measureText(font, text, width, height) || width <= max_width)
    {
        return text;
    }

    if (max_width <= 0)
    {
        return {};
    }

    int ellipsis_width  = 0;
    int ellipsis_height = 0;

    if (!measureText(font, TEXT_ELLIPSIS, ellipsis_width, ellipsis_height))
    {
        return text;
    }

    if (ellipsis_width >= max_width)
    {
        int         fits_width = 0;
        std::size_t fits       = 0;

        if (TTF_MeasureString(font, TEXT_ELLIPSIS.data(), TEXT_ELLIPSIS.size(), max_width, &fits_width, &fits))
        {
            return std::string(TEXT_ELLIPSIS.substr(0, fits));
        }

        return {};
    }

    int         fits_width = 0;
    std::size_t fits       = 0;

    if (!TTF_MeasureString(font, text.c_str(), text.size(), max_width - ellipsis_width, &fits_width, &fits))
    {
        return std::string(TEXT_ELLIPSIS);
    }

    std::string head = text.substr(0, fits);
    while (!head.empty() && (head.back() == ' ' || head.back() == '\t'))
    {
        head.pop_back();
    }

    head.append(TEXT_ELLIPSIS.data(), TEXT_ELLIPSIS.size());

    return head;
}

void updateBlockSize(TextBlock& block)
{
    block.width = 0;

    int text_height = 0;

    for (const TextLine& line : block.lines)
    {
        block.width = std::max(block.width, line.width);
        text_height = std::max(text_height, line.height);
    }

    if (block.lines.empty())
    {
        block.height = 0;
        return;
    }

    block.height = static_cast<int>(std::ceil(static_cast<float>(block.lines.size() - 1U) * block.line_advance +
                                              static_cast<float>(text_height)));
}

TextBlock layoutBlock(TTF_Font* font, const std::string& text, const IRenderer::TextFormat& format, int max_width)
{
    TextBlock block;
    block.line_height  = fontLineHeight(font);
    block.line_advance = std::max(static_cast<float>(block.line_height) + format.line_spacing, 1.0F);

    for (const std::string& paragraph : splitParagraphs(text))
    {
        const std::vector<std::string> paragraph_lines = (format.wrap && max_width > 0) ?
                                                             wrapParagraph(font, paragraph, max_width) :
                                                             std::vector<std::string>{paragraph};

        for (const std::string& paragraph_line : paragraph_lines)
        {
            TextLine line;
            line.text = paragraph_line;

            if (!measureText(font, line.text, line.width, line.height))
            {
                line.width  = 0;
                line.height = block.line_height;
            }

            block.line_height = std::max(block.line_height, line.height);
            block.lines.push_back(std::move(line));
        }
    }

    block.line_advance = std::max(static_cast<float>(block.line_height) + format.line_spacing, 1.0F);
    updateBlockSize(block);

    return block;
}

void applyTailReplacement(TTF_Font* font, TextBlock& block, int max_width, int max_height)
{
    if (max_width > 0)
    {
        for (TextLine& line : block.lines)
        {
            if (line.width > max_width)
            {
                line.text = ellipsize(font, line.text, max_width);
                measureText(font, line.text, line.width, line.height);
            }
        }

        updateBlockSize(block);
    }

    if (max_height <= 0 || block.height <= max_height || block.lines.empty())
    {
        return;
    }

    std::vector<TextLine> visible_lines;
    std::size_t           index = 0;
    float                 y     = 0.0F;

    while (index < block.lines.size() && static_cast<int>(std::floor(y)) + block.line_height <= max_height)
    {
        visible_lines.push_back(block.lines[index]);
        y += block.line_advance;
        ++index;
    }

    if (visible_lines.empty())
    {
        visible_lines.push_back(block.lines.front());
        index = 1;
    }

    if (index < block.lines.size())
    {
        TextLine& last_line = visible_lines.back();
        last_line.text      = ellipsize(font, last_line.text, max_width);
        measureText(font, last_line.text, last_line.width, last_line.height);
    }

    block.lines = std::move(visible_lines);
    updateBlockSize(block);
}

SDL_Texture* createTextTexture(SDL_Renderer* renderer, TTF_Font* font, const TextBlock& block,
                               const IRenderer::Color& color, IRenderer::TextHAlign h_align)
{
    if (renderer == nullptr || block.width <= 0 || block.height <= 0)
    {
        return nullptr;
    }

    SDL_Surface* surface = SDL_CreateSurface(block.width, block.height, SDL_PIXELFORMAT_RGBA32);
    if (surface == nullptr)
    {
        logSdlError("SDL_CreateSurface failed");
        return nullptr;
    }

    SDL_FillSurfaceRect(surface, nullptr, 0U);

    const SDL_Color sdl_color = {color.r, color.g, color.b, color.a};
    float           y         = 0.0F;

    for (const TextLine& line : block.lines)
    {
        if (!line.text.empty())
        {
            SDL_Surface* line_surface = TTF_RenderText_Blended(font, line.text.c_str(), line.text.size(), sdl_color);
            if (line_surface == nullptr)
            {
                logSdlError("TTF_RenderText_Blended failed");
            }
            else
            {
                int x = 0;
                if (h_align == IRenderer::TextHAlign::RIGHT)
                {
                    x = block.width - line_surface->w;
                }
                else if (h_align == IRenderer::TextHAlign::CENTER)
                {
                    x = (block.width - line_surface->w) / 2;
                }

                const SDL_Rect dst = {x, static_cast<int>(std::lround(y)), line_surface->w, line_surface->h};
                if (!SDL_BlitSurface(line_surface, nullptr, surface, &dst))
                {
                    logSdlError("SDL_BlitSurface failed");
                }

                SDL_DestroySurface(line_surface);
            }
        }

        y += block.line_advance;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);

    if (texture == nullptr)
    {
        logSdlError("SDL_CreateTextureFromSurface failed");
        return nullptr;
    }

    if (!SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND))
    {
        logSdlError("SDL_SetTextureBlendMode failed");
    }

    return texture;
}



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

            const auto type =
                sdl_event.type == SDL_EVENT_KEY_DOWN ? EventKeyboard::Type::KEY_DOWN : EventKeyboard::Type::KEY_UP;

            return EventKeyboard(type, key.value(), toModifiers(sdl_event.key.mod));
        }

        case SDL_EVENT_QUIT:
            return IInputPump::EventQuit();

        default:
            return std::nullopt;
    }
}

} // namespace

/* ------------------ private methods ------------------ */

SDL3Engine::SDL3Engine()
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

    openWindow(initialWindowSize());

    ApplicationCore::log(ApplicationCore::LogLevel::OK, "Input pump initialized successfully");
}

SDL3Engine::~SDL3Engine()
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
    ApplicationCore::log(ApplicationCore::LogLevel::OK, "Input pump deinitialized successfully");
}

void SDL3Engine::openWindow(const Vec2F& size)
{
    window_ = SDL_CreateWindow("GDInterface", static_cast<int>(size.x()), static_cast<int>(size.y()),
                               SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
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

    if (!SDL_SetRenderLogicalPresentation(renderer_, LOGICAL_WIDTH, LOGICAL_HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX))
    {
        logSdlError("SDL_SetRenderLogicalPresentation failed");
    }

    if (!SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND))
    {
        logSdlError("SDL_SetRenderDrawBlendMode failed");
    }
}

void SDL3Engine::closeWindow()
{
    SDL_DestroyRenderer(renderer_);
    SDL_DestroyWindow(window_);

    renderer_ = nullptr;
    window_   = nullptr;
}

void SDL3Engine::setRenderColor(const Color& color)
{
    assert(renderer_);

    if (!SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a))
    {
        logSdlError("SDL_SetRenderDrawColor failed");
    }
}

void SDL3Engine::drawPolygon(const std::vector<Vec2F>& points, const Color& color, bool filled)
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
    const RenderBlendModeGuard restore_blend_mode(renderer_);
    const bool ok = filled ?
                        filledPolygonRGBA(renderer_, xs.data(), ys.data(), count, color.r, color.g, color.b, color.a) :
                        polygonRGBA(renderer_, xs.data(), ys.data(), count, color.r, color.g, color.b, color.a);
    if (!ok)
    {
        logSdlError("Polygon drawing failed");
    }
}

TTF_Font* SDL3Engine::fontInstance(FontID font_id, int size)
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

bool SDL3Engine::isValid() const
{
    return window_ != nullptr && renderer_ != nullptr;
}

/* from IRenderer */

std::optional<IRenderer::TextureID> SDL3Engine::loadTexture(const std::string& filename)
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

std::optional<IRenderer::FontID> SDL3Engine::loadFont(const std::string& filename)
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

void SDL3Engine::renderRect(const Color& color, const BoxF& box, float angle)
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

void SDL3Engine::renderRectLines(const Color& color, const BoxF& box, float angle)
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

void SDL3Engine::renderTexture(const TextureID& texture, const BoxF& box, float angle)
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

void SDL3Engine::renderEllipse(const Color& color, const BoxF& box, float angle)
{
    assert(renderer_);

    const Vec2F radius = box.size() / 2.0F;
    const bool  fits_primitive = radius.x() <= MAX_ELLIPSE_PRIMITIVE_RADIUS &&
                                radius.y() <= MAX_ELLIPSE_PRIMITIVE_RADIUS;

    if (angle == 0.0F && fits_primitive)
    {
        const Vec2F center = box.center();
        const RenderBlendModeGuard restore_blend_mode(renderer_);

        if (!filledEllipseRGBA(renderer_, center.x(), center.y(), radius.x(), radius.y(), color.r, color.g, color.b,
                               color.a))
        {
            logSdlError("filledEllipseRGBA failed");
        }
        return;
    }

    drawPolygon(ellipsePoints(box, angle), color, true);
}

void SDL3Engine::renderEllipseLines(const Color& color, const BoxF& box, float angle)
{
    assert(renderer_);

    const Vec2F radius = box.size() / 2.0F;
    const bool  fits_primitive = radius.x() <= MAX_ELLIPSE_PRIMITIVE_RADIUS &&
                                radius.y() <= MAX_ELLIPSE_PRIMITIVE_RADIUS;

    if (angle == 0.0F && fits_primitive)
    {
        const Vec2F center = box.center();
        const RenderBlendModeGuard restore_blend_mode(renderer_);

        if (!ellipseRGBA(renderer_, center.x(), center.y(), radius.x(), radius.y(), color.r, color.g, color.b, color.a))
        {
            logSdlError("ellipseRGBA failed");
        }
        return;
    }

    drawPolygon(ellipsePoints(box, angle), color, false);
}

void SDL3Engine::renderLine(const Color& color, const Vec2F& origin, const Vec2F& end)
{
    assert(renderer_);

    const RenderBlendModeGuard restore_blend_mode(renderer_);

    if (!lineRGBA(renderer_, origin.x(), origin.y(), end.x(), end.y(), color.r, color.g, color.b, color.a))
    {
        logSdlError("lineRGBA failed");
    }
}

void SDL3Engine::renderVector(const Color& color, const Vec2F& origin, const Vec2F& vector)
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

void SDL3Engine::renderPixel(const Color& color, const Vec2F& pos)
{
    assert(renderer_);

    const RenderBlendModeGuard restore_blend_mode(renderer_);

    if (!pixelRGBA(renderer_, pos.x(), pos.y(), color.r, color.g, color.b, color.a))
    {
        logSdlError("pixelRGBA failed");
    }
}

/* from IVertexRenderer */

void SDL3Engine::renderVertex(const Color& color, const Vertex& vertex)
{
    assert(renderer_);

    const RenderBlendModeGuard restore_blend_mode(renderer_);

    if (!filledTrigonRGBA(renderer_, vertex.x0.x(), vertex.x0.y(), vertex.x1.x(), vertex.x1.y(), vertex.x2.x(),
                          vertex.x2.y(), color.r, color.g, color.b, color.a))
    {
        logSdlError("filledTrigonRGBA failed");
    }
}

void SDL3Engine::renderVertexLine(const Color& color, const Vertex& vertex)
{
    assert(renderer_);

    const RenderBlendModeGuard restore_blend_mode(renderer_);

    if (!trigonRGBA(renderer_, vertex.x0.x(), vertex.x0.y(), vertex.x1.x(), vertex.x1.y(), vertex.x2.x(), vertex.x2.y(),
                    color.r, color.g, color.b, color.a))
    {
        logSdlError("trigonRGBA failed");
    }
}

void SDL3Engine::update()
{
    assert(renderer_);

    if (!SDL_RenderPresent(renderer_))
    {
        logSdlError("SDL_RenderPresent failed");
    }

    clear();
}

void SDL3Engine::clear()
{
    assert(renderer_);

    setRenderColor(Color::BLACK);
    if (!SDL_RenderClear(renderer_))
    {
        logSdlError("SDL_RenderClear failed");
    }
}

void SDL3Engine::renderText(const Color& color, const BoxF& box, const std::string& text, const TextFormat& format,
                            float angle)
{
    assert(renderer_);

    if (text.empty())
    {
        return;
    }

    if (format.font_size <= 0)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Font size must be positive, got %d", format.font_size);
        return;
    }

    const float padding    = std::max(format.padding, 0.0F);
    const float left       = std::min(box.left(), box.right()) + padding;
    const float top        = std::min(box.top(), box.bottom()) + padding;
    const float width      = std::abs(box.right() - box.left()) - 2.0F * padding;
    const float height     = std::abs(box.bottom() - box.top()) - 2.0F * padding;
    const int   max_width  = static_cast<int>(std::floor(width));
    const int   max_height = static_cast<int>(std::floor(height));

    if (max_width <= 0 || max_height <= 0)
    {
        return;
    }

    int       font_size = format.font_size;
    TTF_Font* font      = fontInstance(format.font, font_size);
    if (font == nullptr)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Font %llu is not loaded or can not be opened",
                             static_cast<unsigned long long>(format.font));
        return;
    }

    TextBlock block = layoutBlock(font, text, format, max_width);

    if (format.overflow_action == TextOverflowAction::FONTSIZELESS &&
        (block.width > max_width || block.height > max_height))
    {
        int best_size = 0;
        int low       = MIN_FONT_SIZE;
        int high      = font_size - 1;

        while (low <= high)
        {
            const int middle    = low + (high - low) / 2;
            TTF_Font* candidate = fontInstance(format.font, middle);

            if (candidate == nullptr)
            {
                high = middle - 1;
                continue;
            }

            const TextBlock candidate_block = layoutBlock(candidate, text, format, max_width);
            if (candidate_block.width <= max_width && candidate_block.height <= max_height)
            {
                best_size = middle;
                low       = middle + 1;
            }
            else
            {
                high = middle - 1;
            }
        }

        if (best_size > 0)
        {
            TTF_Font* reduced_font = fontInstance(format.font, best_size);

            if (reduced_font != nullptr)
            {
                font  = reduced_font;
                block = layoutBlock(font, text, format, max_width);
            }
        }
    }
    else if (format.overflow_action == TextOverflowAction::TAILREPLACE)
    {
        applyTailReplacement(font, block, max_width, max_height);
    }

    SDL_Texture* texture = createTextTexture(renderer_, font, block, color, format.h_align);
    if (texture == nullptr)
    {
        return;
    }

    float block_x = left;
    float block_y = top;

    switch (format.h_align)
    {
        case TextHAlign::LEFT:
            break;
        case TextHAlign::RIGHT:
            block_x += width - static_cast<float>(block.width);
            break;
        case TextHAlign::CENTER:
            block_x += (width - static_cast<float>(block.width)) / 2.0F;
            break;
    }

    switch (format.w_align)
    {
        case TextWAlign::TOP:
            break;
        case TextWAlign::BOTTOM:
            block_y += height - static_cast<float>(block.height);
            break;
        case TextWAlign::CENTER:
            block_y += (height - static_cast<float>(block.height)) / 2.0F;
            break;
    }

    const SDL_FRect dst = {block_x, block_y, static_cast<float>(block.width), static_cast<float>(block.height)};

    const SDL_Rect clip = {
        static_cast<int>(std::floor(left)),
        static_cast<int>(std::floor(top)),
        static_cast<int>(std::ceil(width)),
        static_cast<int>(std::ceil(height)),
    };
    SDL_SetRenderClipRect(renderer_, &clip);

    if (angle == 0.0F)
    {
        if (!SDL_RenderTexture(renderer_, texture, nullptr, &dst))
        {
            logSdlError("SDL_RenderTexture failed");
        }
    }
    else
    {
        const Vec2F      center          = box.center();
        const SDL_FPoint rotation_center = {center.x(), center.y()};

        if (!SDL_RenderTextureRotated(renderer_, texture, nullptr, &dst, angle * RAD_TO_DEG, &rotation_center,
                                      SDL_FLIP_NONE))
        {
            logSdlError("SDL_RenderTextureRotated failed");
        }
    }

    SDL_SetRenderClipRect(renderer_, nullptr);
    SDL_DestroyTexture(texture);
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
        if (renderer_ != nullptr && !SDL_ConvertEventToRenderCoordinates(renderer_, &sdl_event))
        {
            logSdlError("SDL_ConvertEventToRenderCoordinates failed");
        }

        auto event = convertEvent(sdl_event);
        if (event.has_value())
        {
            return event;
        }
    }

    return std::nullopt;
}

}; // namespace Engine

}; // namespace Kabuto

extern "C" bool INIT_PACKAGE_FUNC_NAME()
{
    Kabuto::Engine::SDL3Engine& engine = Kabuto::Engine::SDL3Engine::instance();
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

    if (ApplicationCore::instance().registerInterface(IID_VERTEX_RENDERER,
                                                      static_cast<Kabuto::IVertexRenderer*>(&engine)) !=
        ApplicationCore::Error::SUCCESS)
    {
        return false;
    }

    return true;
}
