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

inline const Color Color::BLACK{0U, 0U, 0U, 255U};
inline const Color Color::WHITE{255U, 255U, 255U, 255U};
inline const Color Color::RED{255U, 0U, 0U, 255U};
inline const Color Color::GREEN{0U, 255U, 0U, 255U};
inline const Color Color::BLUE{0U, 0U, 255U, 255U};
inline const Color Color::YELLOW{255U, 255U, 0U, 255U};
inline const Color Color::CYAN{0U, 255U, 255U, 255U};
inline const Color Color::MAGENTA{255U, 0U, 255U, 255U};
inline const Color Color::ORANGE{255U, 165U, 0U, 255U};
inline const Color Color::PURPLE{128U, 0U, 128U, 255U};
inline const Color Color::PINK{255U, 192U, 203U, 255U};
inline const Color Color::BROWN{165U, 42U, 42U, 255U};
inline const Color Color::GRAY{128U, 128U, 128U, 255U};
inline const Color Color::LIGHT_GRAY{211U, 211U, 211U, 255U};
inline const Color Color::DARK_GRAY{64U, 64U, 64U, 255U};
inline const Color Color::TRANSPARENT{0U, 0U, 0U, 0U};

class IRenderer
{
public:
    using TextureID = uint64_t;
    using FontID    = uint64_t;

    virtual ~IRenderer() = default;

    virtual std::optional<TextureID> loadTexture(const char* path)  = 0;
    virtual std::optional<FontID>    loadFont(const char* filename) = 0;

    virtual void renderRect(const Color& color, const BoxF& box, float angle = 0)                                   = 0;
    virtual void renderRectLines(const Color& color, const BoxF& box, float angle = 0)                              = 0;
    virtual void renderTexture(const TextureID& texture, const BoxF& box, float angle = 0, bool flip_by_oy = false) = 0;
    virtual void renderEllipse(const Color& color, const BoxF& box, float angle = 0)                                = 0;
    virtual void renderEllipseLines(const Color& color, const BoxF& box, float angle = 0)                           = 0;
    virtual void renderLine(const Vec2F& origin, const Vec2F& end, const Color& color)                              = 0;
    virtual void renderVector(const Vec2F& origin, const Vec2F& vector, const Color& color)                         = 0;
    virtual void renderPixel(const Vec2F& pos, const Color& color)                                                  = 0;

    virtual void update()                                           = 0;
    virtual void clear()                                            = 0;
    virtual void renderText(const char* text, const Color& color, const BoxF& box, float angle, const FontID& font_id,
                            int font_size, bool flip_by_oy = false) = 0;
};

enum class TextHAlign
{
    Left,
    Right,
    Center,
}; // enum TextHAlign

enum class TextWAlign
{
    Top,
    Bottom,
    Center,
}; // enum TextWAlign

enum class TextOverflowAction
{
    CutByBox,
    TailReplace,
    FontSizeLess,
}; // enum TextOverflowAction

struct TextFormat
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

#endif
