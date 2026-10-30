#ifndef I_RENDERER_HPP
#define I_RENDERER_HPP

#include "Interfaces/IUnknown.hpp"
#include <cstdint>

struct BoxF;
struct Vec2f;

struct Color
{
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t a = 0;
};

// TODO: add basic colors (like Color::RED, Color::GREEN, etc)

class IRenderer : public IUnknown
{
public:
    using ResourceID = uint64_t;
    using FontID     = uint64_t;

    virtual ~IRenderer() = default;

    virtual ResourceID loadTexture( const char* path ) = 0;
    virtual FontID loadFont( const char* filename )    = 0;

    virtual void renderRect( const Color& color, const BoxF& box, const float& angle )         = 0;
    virtual void renderRectLines( const Color& color, const BoxF& box, const float& angle )    = 0;
    virtual void renderTexture( const ResourceID& texture, const BoxF& box, const float& angle ) = 0;
    virtual void renderCircle( const Color& color, const BoxF& box )                           = 0;
    virtual void renderEllipse( const Color& color, const BoxF& box )                          = 0;
    virtual void renderEllipseLines( const Color& color, const BoxF& box )                     = 0;
    virtual void renderLine( const Vec2f& origin, const Vec2f& end, const Color& color )       = 0;
    virtual void renderVector( const Vec2f& origin, const Vec2f& vector, const Color& color )  = 0;
    virtual void renderPixel( const Vec2f& pos, const Color& color )                           = 0;

    virtual void update() = 0;
    virtual void clear() = 0;
    virtual void renderText( const char* text, const Color& color, const BoxF& box, const FontID& font_id, int font_size ) = 0;
};

#endif
