#ifndef PHOTOSHOP_IRENDERER_HPP
#define PHOTOSHOP_IRENDERER_HPP

#include <cstdint>

struct BoxF;
struct Vec2f;

struct Color_t
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;

    Color_t(uint8_t r1, uint8_t g1, uint8_t b1, uint8_t a1);
};

struct TextFormat;

class IRenderer
{
public:
    using ResourceID = uint64_t;
    using FontID     = uint64_t;

    virtual ~IRenderer() = default;

    virtual ResourceID loadTexture(const char* path)  = 0;
    virtual FontID     loadFont(const char* filename) = 0;

    virtual void renderRect(const Color_t& color, const BoxF& box, const float& angle)         = 0;
    virtual void renderRectLines(const Color_t& color, const BoxF& box, const float& angle)    = 0;
    virtual void renderTexture(const ResourceID& texture, const BoxF& box, const float& angle) = 0;
    virtual void renderCircle(const Color_t& color, const BoxF& box)                           = 0;
    virtual void renderEllipse(const Color_t& color, const BoxF& box)                          = 0;
    virtual void renderEllipseLines(const Color_t& color, const BoxF& box)                     = 0;
    virtual void renderLine(const Vec2f& origin, const Vec2f& end, const Color_t& color)       = 0;
    virtual void renderVector(const Vec2f& origin, const Vec2f& vector, const Color_t& color)  = 0;
    virtual void renderPixel(const Vec2f& pos, const Color_t& color)                           = 0;

    virtual void update()                                                                = 0;
    virtual void clear()                                                                 = 0;
    virtual void renderText(const char* text, const TextFormat& format, const BoxF& box) = 0;
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
    Color_t            color;
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
