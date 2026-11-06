#ifndef I_RENDERER_HPP
#define I_RENDERER_HPP

#include "Interfaces/IID.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/Vec2F.hpp"

#include <cstdint>

constexpr static const IID IID_RENDERER = 0;

struct Color
{
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t a = 0;
};

enum class Flip : uint8_t
{
    None       = 0,
    Horizontal = 1U << 0,
    Vertical   = 1U << 1,
    Both       = (1U << 0) | (1U << 1),
};

constexpr Flip operator|(Flip lhs, Flip rhs) noexcept
{
    return static_cast<Flip>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

constexpr bool hasFlip(Flip value, Flip flag) noexcept
{
    return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0U;
}

// TODO: add basic colors (like Color::RED, Color::GREEN, etc)

class IRenderer
{
public:
    using TextureID = uint64_t;
    using FontID    = uint64_t;

    virtual ~IRenderer() = default;

    virtual std::optional<TextureID> loadTexture(const char* path)  = 0;
    virtual std::optional<FontID>    loadFont(const char* filename) = 0;

    virtual void renderRect(const Color& color, const BoxF& box, float angle = 0)                                  = 0;
    virtual void renderRectLines(const Color& color, const BoxF& box, float angle = 0)                             = 0;
    virtual void renderTexture(const TextureID& texture, const BoxF& box, float angle = 0, Flip flip = Flip::None) = 0;
    virtual void renderEllipse(const Color& color, const BoxF& box, float angle = 0)                               = 0;
    virtual void renderEllipseLines(const Color& color, const BoxF& box, float angle = 0)                          = 0;
    virtual void renderLine(const Vec2F& origin, const Vec2F& end, const Color& color)                             = 0;
    virtual void renderVector(const Vec2F& origin, const Vec2F& vector, const Color& color)                        = 0;
    virtual void renderPixel(const Vec2F& pos, const Color& color)                                                 = 0;

    virtual void update()                                          = 0;
    virtual void clear()                                           = 0;
    virtual void renderText(const char* text, const Color& color, const BoxF& box, float angle, const FontID& font_id,
                            int font_size, Flip flip = Flip::None) = 0;
};

#endif
