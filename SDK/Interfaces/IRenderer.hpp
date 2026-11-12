#ifndef I_RENDERER_HPP
#define I_RENDERER_HPP

#include "Interfaces/IID.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/Vec2F.hpp"

#include <cstdint>
#include <optional>

constexpr static const IID IID_RENDERER = 0;

class IRenderer
{
public:
    using TextureID = uint64_t;
    using FontID    = uint64_t;
    struct Color;
    struct TextFormat;
    enum class TextHAlign;
    enum class TextWAlign;
    enum class TextOverflowAction;

public:
    virtual ~IRenderer() = default;

    virtual std::optional<TextureID> loadTexture(const std::string& filename) = 0;
    virtual std::optional<FontID>    loadFont(const std::string& filename)    = 0;

    virtual void renderRect(const Color& color, const BoxF& box, float angle = 0)           = 0;
    virtual void renderRectLines(const Color& color, const BoxF& box, float angle = 0)      = 0;
    virtual void renderTexture(const TextureID& texture, const BoxF& box, float angle = 0)  = 0;
    virtual void renderEllipse(const Color& color, const BoxF& box, float angle = 0)        = 0;
    virtual void renderEllipseLines(const Color& color, const BoxF& box, float angle = 0)   = 0;
    virtual void renderLine(const Color& color, const Vec2F& origin, const Vec2F& end)      = 0;
    virtual void renderVector(const Color& color, const Vec2F& origin, const Vec2F& vector) = 0;
    virtual void renderPixel(const Color& color, const Vec2F& pos)                          = 0;

    virtual void update()                    = 0;
    virtual void clear()                     = 0;
    virtual void renderText(const Color& color, const BoxF& box, const std::string& text, const TextFormat& format,
                            float angle = 0) = 0;
};

enum class IRenderer::TextHAlign
{
    LEFT,
    RIGHT,
    CENTER,
}; // enum TextHAlign

enum class IRenderer::TextWAlign
{
    TOP,
    BOTTOM,
    CENTER,
}; // enum TextWAlign

enum class IRenderer::TextOverflowAction
{
    CUTBYBOX,
    TAILREPLACE,
    FONTSIZELESS,
}; // enum TextOverflowAction

struct IRenderer::TextFormat
{
    IRenderer::FontID  font;
    int                font_size;
    TextOverflowAction overflow_action;
    TextHAlign         h_align;
    TextWAlign         w_align;
    float              padding;
    float              line_spacing;
    bool               wrap;
}; // struct TextFormat

struct IRenderer::Color
{
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t a = 0;

    static const Color BLACK;
    static const Color WHITE;
    static const Color RED;
    static const Color GREEN;
    static const Color BLUE;
    static const Color YELLOW;
    static const Color CYAN;
    static const Color MAGENTA;
    static const Color ORANGE;
    static const Color PURPLE;
    static const Color PINK;
    static const Color BROWN;
    static const Color GRAY;
    static const Color LIGHT_GRAY;
    static const Color DARK_GRAY;
    static const Color TRANSPARENT;
};

inline constexpr IRenderer::Color IRenderer::Color::BLACK{0U, 0U, 0U, 255U};
inline constexpr IRenderer::Color IRenderer::Color::WHITE{255U, 255U, 255U, 255U};
inline constexpr IRenderer::Color IRenderer::Color::RED{255U, 0U, 0U, 255U};
inline constexpr IRenderer::Color IRenderer::Color::GREEN{0U, 255U, 0U, 255U};
inline constexpr IRenderer::Color IRenderer::Color::BLUE{0U, 0U, 255U, 255U};
inline constexpr IRenderer::Color IRenderer::Color::YELLOW{255U, 255U, 0U, 255U};
inline constexpr IRenderer::Color IRenderer::Color::CYAN{0U, 255U, 255U, 255U};
inline constexpr IRenderer::Color IRenderer::Color::MAGENTA{255U, 0U, 255U, 255U};
inline constexpr IRenderer::Color IRenderer::Color::ORANGE{255U, 165U, 0U, 255U};
inline constexpr IRenderer::Color IRenderer::Color::PURPLE{128U, 0U, 128U, 255U};
inline constexpr IRenderer::Color IRenderer::Color::PINK{255U, 192U, 203U, 255U};
inline constexpr IRenderer::Color IRenderer::Color::BROWN{165U, 42U, 42U, 255U};
inline constexpr IRenderer::Color IRenderer::Color::GRAY{128U, 128U, 128U, 255U};
inline constexpr IRenderer::Color IRenderer::Color::LIGHT_GRAY{211U, 211U, 211U, 255U};
inline constexpr IRenderer::Color IRenderer::Color::DARK_GRAY{64U, 64U, 64U, 255U};
inline constexpr IRenderer::Color IRenderer::Color::TRANSPARENT{0U, 0U, 0U, 0U};

#endif
